#include "pch.h"
#include "Player.h"
#include "Inventory.h"
#include "EquippedGear.h"

Player::Player()
{
	_isPlayer = true;

    playerInfo = objectInfo->mutable_player_info();
    statInfo = playerInfo->mutable_stat_info();

    inventory = make_shared<Inventory>(static_pointer_cast<Player>(shared_from_this()));
    equippedGear = make_shared<EquippedGear>(static_pointer_cast<Player>(shared_from_this()));
}

Player::~Player()
{
}

bool Player::Init()
{
    // 최종 스텟 계산 + playerInfo에 계산 결과 채워넣기
    bool success = CalculateFinalStat();

    return success;
}

bool Player::CalculateFinalStat()
{
    struct FinalStat
    {
        int32 maxHp = 0;
        int32 maxMp = 0;
        int32 physical_attack = 0;
        int32 magical_attack = 0;
    } finalStat;

    // 스킬 패시브, 내실 등 캐릭터 스텟을 올릴 수 있는 요소가 추가되면 여기에 작성...
    // ===========================================================================
    
    // 1. 레벨당 캐릭터 기본 스텟
    DataTable& classLevelDataTable = (*Gamedata::ClassLevelDataTableMappings[playerInfo->class_()]);
    int32 level = playerInfo->level();
    finalStat.maxHp += classLevelDataTable[level]["maxHp"];
    finalStat.maxMp += classLevelDataTable[level]["maxMp"];
    finalStat.physical_attack += classLevelDataTable[level]["physicalAttack"];
    finalStat.magical_attack += classLevelDataTable[level]["magicalAttack"];

    // 2. 장착 중이 장비 스텟 추가
    for (const auto& slot : playerInfo->equipped_gear())
    {
        const Protocol::Item& item = slot.item();
        finalStat.maxHp += Gamedata::ItemDataTable[item.template_id()]["hp"];
        finalStat.maxMp += Gamedata::ItemDataTable[item.template_id()]["mp"];
        finalStat.physical_attack += Gamedata::ItemDataTable[item.template_id()]["physicalAttack"];
        finalStat.magical_attack += Gamedata::ItemDataTable[item.template_id()]["magicalAttack"];
    }

    // validate
    try
    {
        if (statInfo->hp() > finalStat.maxHp)
            throw wstring(L"현재 HP가 최대 HP를 초과");
        if (statInfo->mp() > finalStat.maxMp)
            throw wstring(L"현재 MP가 최대 MP를 초과");
        if(statInfo->physical_attack() != finalStat.physical_attack)
            throw wstring(L"물리 공격력이 계산 결과와 일치하지 않음");
        if (statInfo->magical_attack() != finalStat.magical_attack)
            throw wstring(L"마법 공격력이 계산 결과와 일치하지 않음");
    }
    catch (wstring& cause)
    {
        wcerr << L"스텟 계산에 문제가 생겼습니다. 사유: " << cause << endl;
        return false;
    }

    // Protocol::statInfo에 최종 스텟 적용
    statInfo->set_max_hp(finalStat.maxHp);
    statInfo->set_max_mp(finalStat.maxMp);
    statInfo->set_physical_attack(finalStat.physical_attack);
    statInfo->set_magical_attack(finalStat.magical_attack);

    return true;
}

bool Player::BuyItem(OUT Protocol::Slot* updatedSlot, OUT int64& totalGold, int32 templateId, int32 count)
{
    int64 gold = playerInfo->gold();
    int64 buyPrice = Gamedata::ItemDataTable[templateId]["buyPrice"] * count;

    if (gold < buyPrice)
        return false;

    if (inventory->addItem(OUT updatedSlot, templateId, count) == false)
        return false;

    totalGold = gold - buyPrice;
    playerInfo->set_gold(totalGold);

    return true;
}

bool Player::SellItem(OUT Protocol::Slot* updatedSlot, Protocol::Slot* targetSlot, OUT int64& totalGold, int32 count)
{
    int64 gold = playerInfo->gold();
    int32 templateId = targetSlot->item().template_id();
    int64 sellPrice = Gamedata::ItemDataTable[templateId]["sellPrice"] * count;

    if (inventory->removeItem(OUT updatedSlot, targetSlot, count) == false)
        return false;

    totalGold = gold + sellPrice;
    playerInfo->set_gold(totalGold);

    return true;
}

bool Player::EquipGear(OUT Protocol::S_EQUIP_GEAR& pkt, Protocol::Slot* targetSlot)
{
    Protocol::Slot* updatedSlot = nullptr;
    Protocol::StatInfo* updatedStatInfo = pkt.mutable_updated_stat_info();

    if (targetSlot == nullptr || targetSlot->has_item() == false)
        return false;

    updatedSlot = pkt.add_updated_slots();
    Protocol::Item* itemInstance = targetSlot->mutable_item();
    if (equippedGear->EquipGear(OUT updatedSlot, OUT statInfo, *itemInstance) == false)
        return false;

    updatedStatInfo->CopyFrom(*statInfo);

    updatedSlot = pkt.add_updated_slots();
    if (inventory->addItem(OUT updatedSlot, *(targetSlot->mutable_item())) == false)
        return false;

    return true;
}

bool Player::UnequipGear(OUT Protocol::S_UNEQUIP_GEAR& pkt, Protocol::Slot* targetSlot)
{
    Protocol::Slot* updatedSlot = nullptr;
    Protocol::StatInfo* updatedStatInfo = pkt.mutable_updated_stat_info();

    updatedSlot = pkt.add_updated_slots();
    if (equippedGear->UnequipGear(OUT updatedSlot, OUT statInfo, targetSlot) == false)
        return false;

    updatedStatInfo->CopyFrom(*statInfo);

    updatedSlot = pkt.add_updated_slots();
    if (inventory->removeItem(OUT updatedSlot, targetSlot) == false)
        return false;

    return true;
}
