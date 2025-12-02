#include "pch.h"
#include "Player.h"
#include "Inventory.h"
#include "EquippedGear.h"
#include "Monster.h"
#include "Room.h"

Player::Player()
{
	_isPlayer = true;

    playerInfo = objectInfo->mutable_player_info();
    statInfo = playerInfo->mutable_stat_info();
}

Player::~Player()
{
}

void Player::PostConstructionSetup()
{
    Creature::PostConstructionSetup();

    Init();
}

void Player::Tick(float deltaTime)
{
    Creature::Tick(deltaTime);


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

    CacheNextLevelUpData();

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
    if(classLevelDataTable[level].contains(JsonProperty::LevelTable::MaxHp))
        finalStat.maxHp += static_cast<int32>(classLevelDataTable[level][JsonProperty::LevelTable::MaxHp]);
    if (classLevelDataTable[level].contains(JsonProperty::LevelTable::MaxMp))
        finalStat.maxMp += static_cast<int32>(classLevelDataTable[level][JsonProperty::LevelTable::MaxMp]);
    if (classLevelDataTable[level].contains(JsonProperty::LevelTable::PhysicalAttack))
        finalStat.physical_attack += static_cast<int32>(classLevelDataTable[level][JsonProperty::LevelTable::PhysicalAttack]);
    if (classLevelDataTable[level].contains(JsonProperty::LevelTable::MagicalAttack))
        finalStat.magical_attack += static_cast<int32>(classLevelDataTable[level][JsonProperty::LevelTable::MagicalAttack]);

    // 2. 장착 중이 장비 스텟 추가
    for (const auto& pair : playerInfo->equipped_gear())
    {
        const Protocol::Item& item = pair.second.item();

        if (item.template_id() == 0)
            continue;

        if (Gamedata::ItemDataTable[item.template_id()].contains(JsonProperty::Item::Hp))
            finalStat.maxHp += static_cast<int32>(Gamedata::ItemDataTable[item.template_id()][JsonProperty::Item::Hp]);
        if (Gamedata::ItemDataTable[item.template_id()].contains(JsonProperty::Item::Mp))
            finalStat.maxMp += static_cast<int32>(Gamedata::ItemDataTable[item.template_id()][JsonProperty::Item::Mp]);
        if (Gamedata::ItemDataTable[item.template_id()].contains(JsonProperty::Item::PhysicalAttack))
            finalStat.physical_attack += static_cast<int32>(Gamedata::ItemDataTable[item.template_id()][JsonProperty::Item::PhysicalAttack]);
        if (Gamedata::ItemDataTable[item.template_id()].contains(JsonProperty::Item::MagicalAttack))
            finalStat.magical_attack += static_cast<int32>(Gamedata::ItemDataTable[item.template_id()][JsonProperty::Item::MagicalAttack]);
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
    int64 buyPrice = static_cast<int64>(Gamedata::ItemDataTable[templateId][JsonProperty::Item::BuyPrice]) * count;

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
    int64 sellPrice = static_cast<int64>(Gamedata::ItemDataTable[templateId][JsonProperty::Item::SellPrice]) * count;

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
    
    if (itemData.contains(JsonProperty::Item::HpRestore))
    {
        float ratio = itemData[JsonProperty::Item::HpRestore];
        int32 amount = (int32)(statInfo->max_hp() * ratio);
        int32 updatedHp = min(statInfo->max_hp(), statInfo->hp() + amount);
        statInfo->set_hp(updatedHp);
        updatedStatInfo->set_hp(updatedHp);
    }

    if (itemData.contains(JsonProperty::Item::MpRestore))
    {
        float ratio = itemData[JsonProperty::Item::MpRestore];
        int32 amount = (int32)(statInfo->max_mp() * ratio);
        int32 updatedMp = min(statInfo->max_mp(), statInfo->mp() + amount);
        statInfo->set_mp(updatedMp);
        updatedStatInfo->set_mp(updatedMp);
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

void Player::OnHit(ObjectRef attacker, Protocol::HitData& hitData)
{
    auto ownerRoom = room.load().lock();
    if (ownerRoom == nullptr)
        return;

    uint64 damage = hitData.damage();
    int32 updated_hp = static_cast<int32>(statInfo->hp() - damage);
    statInfo->set_hp(max(0, updated_hp));

    if (updated_hp > 0)
    {
        // Send Hit Packet
        {
            Protocol::S_HIT hitPkt;

            hitPkt.mutable_hit_data()->CopyFrom(hitData);
            hitPkt.set_hp(updated_hp);

            if (auto ownerSession = session.lock())
            {
                SEND_PACKET_USING_THIS_SESSION(ownerSession, hitPkt);
            }

        }
    }
    else
    {
        // 죽으면 경험치 10% 감소
        uint64 lossExp = static_cast<uint64>(playerInfo->max_exp() * 0.1);
        uint64 curExp = playerInfo->cur_exp();
        uint64 updatedExp = curExp <= lossExp ? 0 : curExp - lossExp;

        playerInfo->set_cur_exp(updatedExp);

        // Make and Pass On Die Packet
        Protocol::S_DIE DiePkt;
        {
            DiePkt.set_object_id(objectInfo->object_id());
            DiePkt.mutable_die_penalty_details()->set_updated_exp(updatedExp);
        }

        ownerRoom->OnDie(DiePkt);
    }
}

void Player::OnMonsterKill(MonsterRef killedMonster, uint64 expReward, uint64 goldReward)
{
    bool levelUp = false;

    // Get Reward
    {
        playerInfo->set_cur_exp(playerInfo->cur_exp() + expReward);
        playerInfo->set_gold(playerInfo->gold() + goldReward);

        // Check Level Up
        if (playerInfo->cur_exp() >= playerInfo->max_exp())
        {
            playerInfo->set_cur_exp(playerInfo->cur_exp() - playerInfo->max_exp());
            OnLevelUp();
            levelUp = true;
        }
    }

    // Send S_MONSTER_KILL_RESULT Packet
    Protocol::S_MONSTER_KILL_RESULT monsterKillResultPkt;
    {
        monsterKillResultPkt.set_object_id(objectInfo->object_id());
        monsterKillResultPkt.set_monster_object_id(killedMonster->objectInfo->object_id());

        monsterKillResultPkt.set_current_exp(playerInfo->cur_exp());
        monsterKillResultPkt.set_current_gold(playerInfo->gold());

        if (levelUp)
        {
            monsterKillResultPkt.set_is_level_up(true);
            Protocol::LevelUpInfo info;
            info.set_new_level(playerInfo->level() - 1);
            info.set_new_level(playerInfo->level());

            info.set_new_max_hp(statInfo->max_hp());
            info.set_new_max_mp(statInfo->max_hp());
            info.set_new_physical_attack(statInfo->physical_attack());
            info.set_new_magical_attack(statInfo->magical_attack());
        }
    }

    auto ownerSession = session.lock();
    if (ownerSession == nullptr)
        return;

    SEND_PACKET_USING_THIS_SESSION(ownerSession, monsterKillResultPkt);
}

void Player::OnLevelUp()
{
    // Set PlayerInfo
    playerInfo->set_level(playerInfo->level() + 1);
    playerInfo->set_max_exp(_nextLevelUpData.expRequirement);

    // Set StatInfo
    statInfo->set_max_hp(statInfo->max_hp() + (int32)_nextLevelUpData.maxHpIncrement);
    statInfo->set_max_mp(statInfo->max_mp() + (int32)_nextLevelUpData.maxMpIncrement);
    statInfo->set_physical_attack(statInfo->physical_attack() + (int32)_nextLevelUpData.paIncrement);
    statInfo->set_magical_attack(statInfo->magical_attack() + (int32)_nextLevelUpData.maIncrement);

    CacheNextLevelUpData();
}

void Player::SetRespawnHp()
{
    int32 respawnHp = static_cast<int32>(statInfo->max_hp() * 0.5f);
    statInfo->set_hp(respawnHp);
}

void Player::CacheNextLevelUpData()
{
    int32 nextLevel = playerInfo->level() + 1;
    if (nextLevel > (int32)MAX_LEVEL)
    {
        cout << "Current Level is Max! Can't Cache Level Up Data" << '\n';
        return;
    }

    DataTable& classLevelDataTable = (*Gamedata::ClassLevelDataTableMappings[playerInfo->class_()]);
    const Json& nextLevelData = classLevelDataTable[nextLevel];

    // Cache
    {
        using namespace JsonProperty::LevelTable;

        _nextLevelUpData.level = nextLevelData[Level].is_null() ? 0 : static_cast<uint32>(nextLevelData[Level]);
        _nextLevelUpData.maxHpIncrement = nextLevelData[MaxHp_Increment].is_null() ? 0 : static_cast<uint64>(nextLevelData[MaxHp_Increment]);
        _nextLevelUpData.maxMpIncrement = nextLevelData[MaxMp_Increment].is_null() ? 0 : static_cast<uint64>(nextLevelData[MaxMp_Increment]);
        _nextLevelUpData.paIncrement = nextLevelData[PA_Increment].is_null() ? 0 : static_cast<uint64>(nextLevelData[PA_Increment]);
        _nextLevelUpData.maIncrement = nextLevelData[MA_Increment].is_null() ? 0 : static_cast<uint64>(nextLevelData[MA_Increment]);
        _nextLevelUpData.expRequirement = nextLevelData[ExpRequirement].is_null() ? 0 : static_cast<uint64>(nextLevelData[ExpRequirement]);
    }
}
