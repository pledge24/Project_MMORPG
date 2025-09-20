#include "pch.h"
#include "Player.h"
#include "Inventory.h"
#include "EquippedGear.h"

Player::Player()
{
	_isPlayer = true;

    playerInfo = objectInfo->mutable_player_info();
    statInfo = playerInfo->mutable_stat_info();
}

Player::~Player()
{
}

void Player::Init()
{
    inventory = make_shared<Inventory>(static_pointer_cast<Player>(shared_from_this()));
    equippedGear = make_shared<EquippedGear>(static_pointer_cast<Player>(shared_from_this()));
}

bool Player::PostInit()
{
    if (CalculateFinalStat() == false)
        return false;

    return true;
}

bool Player::CalculateFinalStat()
{
    // 최종 스텟 계산 + playerInfo에 계산 결과 채워넣기
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
    if(classLevelDataTable[level].contains("maxHp"))
        finalStat.maxHp += classLevelDataTable[level]["maxHp"];
    if (classLevelDataTable[level].contains("maxMp"))
        finalStat.maxMp += classLevelDataTable[level]["maxMp"];
    if (classLevelDataTable[level].contains("physicalAttack"))
        finalStat.physical_attack += classLevelDataTable[level]["physicalAttack"];
    if (classLevelDataTable[level].contains("magicalAttack"))
        finalStat.magical_attack += classLevelDataTable[level]["magicalAttack"];

    // 2. 장착 중이 장비 스텟 추가
    for (const auto& pair : playerInfo->equipped_gear())
    {
        const Protocol::Item& item = pair.second.item();

        if (item.template_id() == 0)
            continue;

        if (Gamedata::ItemDataTable[item.template_id()].contains("hp"))
            finalStat.maxHp += Gamedata::ItemDataTable[item.template_id()]["hp"];
        if (Gamedata::ItemDataTable[item.template_id()].contains("mp"))
            finalStat.maxMp += Gamedata::ItemDataTable[item.template_id()]["mp"];
        if (Gamedata::ItemDataTable[item.template_id()].contains("physicalAttack"))
            finalStat.physical_attack += Gamedata::ItemDataTable[item.template_id()]["physicalAttack"];
        if (Gamedata::ItemDataTable[item.template_id()].contains("magicalAttack"))
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

bool Player::HandleBuyItem(OUT Protocol::Slot* updatedSlot, OUT int64& totalGold, int32 templateId, int32 count)
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

bool Player::HandleSellItem(OUT Protocol::Slot* updatedSlot, Protocol::Slot* targetSlot, OUT int64& totalGold, int32 count)
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

bool Player::HandleUseItem(OUT Protocol::S_USE_ITEM& pkt, Protocol::Slot* targetSlot)
{
    // 아이템 사용으로 인한 슬롯 변경 정보 채우기
    if (inventory->removeItem(OUT pkt.mutable_updated_inventory_slot(), targetSlot) == false)
        return false;

    // objectId 채우기
    ObjectRef object = shared_from_this();
    pkt.set_object_id(object->objectInfo->object_id());

    // 변경된 스텟 반영
    Protocol::StatInfo* updatedStatInfo = pkt.mutable_updated_stat_info();
    int32 templateId = targetSlot->item().template_id();
    const Json& itemData = Gamedata::ItemDataTable[templateId];
    
    if (itemData.contains("hpRestore"))
    {
        int32 amount = statInfo->max_hp() * itemData["hpRestore"];
        int32 updatedHp = min(statInfo->max_hp(), statInfo->hp() + amount);
        statInfo->set_hp(updatedHp);
        updatedStatInfo->set_hp(updatedHp);
    }

    if (itemData.contains("mpRestore"))
    {
        int32 amount = statInfo->max_mp() * itemData["mpRestore"];
        int32 updatedMp = min(statInfo->max_mp(), statInfo->mp() + amount);
        statInfo->set_mp(updatedMp);
        updatedStatInfo->set_hp(updatedMp);
    }

    return true;
}

bool Player::HandleEquipGear(OUT Protocol::S_EQUIP_GEAR& pkt, Protocol::Slot* targetSlot)
{
    Protocol::Slot* updatedSlot = nullptr;
    Protocol::StatInfo* updatedStatInfo = pkt.mutable_updated_stat_info();

    if (targetSlot == nullptr || targetSlot->has_item() == false)
        return false;

    Protocol::Item* itemInstance = targetSlot->mutable_item();
    if (equippedGear->EquipGear(OUT pkt.mutable_updated_equipped_slot(), OUT statInfo, *itemInstance) == false)
        return false;

    updatedStatInfo->CopyFrom(*statInfo);

    if (inventory->removeItem(OUT pkt.mutable_updated_inventory_slot(), targetSlot) == false)
        return false;

    return true;
}

bool Player::HandleUnequipGear(OUT Protocol::S_UNEQUIP_GEAR& pkt, Protocol::Slot* targetSlot)
{
    Protocol::Slot* updatedSlot = nullptr;
    Protocol::StatInfo* updatedStatInfo = pkt.mutable_updated_stat_info();

    if (equippedGear->UnequipGear(OUT pkt.mutable_updated_equipped_slot(), OUT statInfo, targetSlot) == false)
        return false;

    updatedStatInfo->CopyFrom(*statInfo);

    if (inventory->addItem(OUT pkt.mutable_updated_inventory_slot(), *(targetSlot->mutable_item())) == false)
        return false;

    return true;
}
