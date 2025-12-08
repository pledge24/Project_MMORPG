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
    statInfo = new Protocol::StatInfo();
    possession = new Protocol::Possession();
}

Player::~Player()
{
    delete statInfo;
    delete possession;
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

    respawnRoomMappings[Protocol::RESPAWN_TYPE_TOWN] = RESPAWN_TOWN_ID;
    respawnRoomMappings[Protocol::RESPAWN_TYPE_CHECKPOINT] = -1;
    respawnRoomMappings[Protocol::RESPAWN_TYPE_IN_PLACE] = -1;
    respawnRoomMappings[Protocol::RESPAWN_TYPE_GUILD_BASE] = -1;

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
    for (const auto& pair : possession->equipped_gear())
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

    auto* statMappings = statInfo->mutable_info();

    // validate
    try
    {
        if (statMappings->at((int32)Protocol::STAT_TYPE_MAX_HP) > finalStat.maxHp)
            throw wstring(L"현재 HP가 최대 HP를 초과");
        if (statMappings->at((int32)Protocol::STAT_TYPE_MAX_MP) > finalStat.maxMp)
            throw wstring(L"현재 MP가 최대 MP를 초과");
        if(statMappings->at((int32)Protocol::STAT_TYPE_PHYSICAL_ATTACK) != finalStat.physical_attack)
            throw wstring(L"물리 공격력이 계산 결과와 일치하지 않음");
        if (statMappings->at((int32)Protocol::STAT_TYPE_MAGICAL_ATTACK) != finalStat.magical_attack)
            throw wstring(L"마법 공격력이 계산 결과와 일치하지 않음");
    }
    catch (wstring& cause)
    {
        wcerr << L"스텟 계산에 문제가 생겼습니다. 사유: " << cause << endl;
        return false;
    }

    // Protocol::statInfo에 최종 스텟 적용
    (*statMappings)[(int32)Protocol::STAT_TYPE_MAX_HP] = finalStat.maxHp;
    (*statMappings)[(int32)Protocol::STAT_TYPE_MAX_MP] = finalStat.maxMp;
    (*statMappings)[(int32)Protocol::STAT_TYPE_PHYSICAL_ATTACK] = finalStat.physical_attack;
    (*statMappings)[(int32)Protocol::STAT_TYPE_MAGICAL_ATTACK] = finalStat.magical_attack;

    return true;
}

bool Player::HandleBuyItem(const Protocol::C_BUY_ITEM& pkt)
{
    auto ownerSession = session.lock();
    if (ownerSession == nullptr)
        return false;

    Protocol::S_BUY_ITEM buyItemPkt;
    Protocol::Slot* updatedSlot = buyItemPkt.mutable_updated_slot();
    int32 templateId = pkt.template_id();
    int64 totalGold = 0;

    if (ProcessBuyItem(OUT updatedSlot, OUT totalGold, templateId) == false)
    {
        buyItemPkt.set_success(false);

        SEND_PACKET_USING_THIS_SESSION(ownerSession, buyItemPkt);
        return false;
    }

    // 아이템 구매 성공 처리
    {
        buyItemPkt.set_success(true);
        buyItemPkt.set_gold(totalGold);

        SEND_PACKET_USING_THIS_SESSION(ownerSession, buyItemPkt);
        cout << buyItemPkt.DebugString() << endl;
    }

    return true;
}

bool Player::HandleSellItem(const Protocol::C_SELL_ITEM& pkt)
{
    auto ownerSession = session.lock();
    if (ownerSession == nullptr)
        return false;

    Protocol::S_SELL_ITEM sellItemPkt;
    const Protocol::Slot& requestSlot = pkt.slot();
    Protocol::Slot* updatedSlot = sellItemPkt.mutable_updated_slot();
    int64 totalGold = 0;

    if (ProcessSellItem(requestSlot, OUT updatedSlot, OUT totalGold) == false)
    {
        sellItemPkt.set_success(false);
        SEND_PACKET_USING_THIS_SESSION(ownerSession, sellItemPkt);
        return false;
    }

    // 아이템 판매 성공 처리
    {
        sellItemPkt.set_success(true);
        sellItemPkt.set_gold(totalGold);

        SEND_PACKET_USING_THIS_SESSION(ownerSession, sellItemPkt);
        cout << sellItemPkt.DebugString() << endl;
    }

    return true;
}

bool Player::HandleUseItem(const Protocol::C_USE_ITEM& pkt)
{
    auto ownerSession = session.lock();
    if (ownerSession == nullptr)
        return false;

    Protocol::S_USE_ITEM useItemPkt;
    const Protocol::Slot& targetSlot = pkt.slot();

    if (ProcessUseItem(targetSlot, OUT useItemPkt) == false)
    {
        useItemPkt.set_success(false);

        SEND_PACKET_USING_THIS_SESSION(ownerSession, useItemPkt);
        return false;
    }

    // 아이템 판매 성공 처리
    {
        useItemPkt.set_success(true);

        SEND_PACKET_USING_THIS_SESSION(ownerSession, OUT useItemPkt);
        cout << useItemPkt.DebugString() << endl;
    }

    return true;
}

bool Player::ProcessBuyItem(OUT Protocol::Slot* updatedSlot, OUT int64& totalGold, int32 templateId, int32 count)
{
    int64 gold = possession->gold();
    int64 buyPrice = static_cast<int64>(Gamedata::ItemDataTable[templateId][JsonProperty::Item::BuyPrice]) * count;

    if (gold < buyPrice)
        return false;

    if (inventory->addItem(OUT updatedSlot, templateId, count) == false)
        return false;

    totalGold = gold - buyPrice;
    possession->set_gold(totalGold);

    return true;
}

bool Player::ProcessSellItem(const Protocol::Slot& requestSlot, OUT Protocol::Slot* updatedSlot, OUT int64& totalGold, int32 count)
{
    int64 gold = possession->gold();
    int32 templateId = requestSlot.item().template_id();
    int64 sellPrice = static_cast<int64>(Gamedata::ItemDataTable[templateId][JsonProperty::Item::SellPrice]) * count;

    if (inventory->removeItem(requestSlot, OUT updatedSlot, count) == false)
        return false;

    totalGold = gold + sellPrice;
    possession->set_gold(totalGold);

    return true;
}

bool Player::ProcessUseItem(const Protocol::Slot& requestSlot, OUT Protocol::S_USE_ITEM& pkt)
{
    auto* updatedSlotList = pkt.mutable_updated_slots();
    auto* updatedStatList = pkt.mutable_updated_stat();

    // 아이템 사용으로 인한 슬롯 변경 정보 채우기
    if (inventory->removeItem(requestSlot, OUT updatedSlotList->Add()) == false)
        return false;

    // objectId 채우기
    pkt.set_object_id(objectInfo->object_id());

    // 변경된 스텟 반영
    for (const Protocol::Stat& stat : pkt.updated_stat())
    {
        int32 templateId = requestSlot.item().template_id();
        const Json& itemData = Gamedata::ItemDataTable[templateId];

        // HP
        if (itemData.contains(JsonProperty::Item::HpRestore))
        {
            float ratio = itemData[JsonProperty::Item::HpRestore];
            int64 maxHp = GetStatValue(Protocol::STAT_TYPE_MAX_HP);
            int64 hp = GetStatValue(Protocol::STAT_TYPE_HP);
            int64 amount = static_cast<int64>(maxHp * ratio);
            int64 updatedHp = min(maxHp, hp + amount);

            // Set Updated stat
            SetStatValue(Protocol::STAT_TYPE_HP, updatedHp);
            Protocol::Stat* updatedStat = updatedStatList->Add();
            {
                updatedStat->set_type(Protocol::STAT_TYPE_HP);
                updatedStat->set_value(updatedHp);
            }
        }

        // MP
        if (itemData.contains(JsonProperty::Item::MpRestore))
        {
            float ratio = itemData[JsonProperty::Item::MpRestore];
            int64 maxMp = GetStatValue(Protocol::STAT_TYPE_MAX_MP);
            int64 mp = GetStatValue(Protocol::STAT_TYPE_MP);
            int64 amount = static_cast<int64>(maxMp * ratio);
            int64 updatedMp = min(maxMp, mp + amount);

            // Set Updated stat
            SetStatValue(Protocol::STAT_TYPE_MP, updatedMp);
            Protocol::Stat* updatedStat = updatedStatList->Add();
            {
                updatedStat->set_type(Protocol::STAT_TYPE_MP);
                updatedStat->set_value(updatedMp);
            }
        }
    }

    return true;
}

bool Player::ProcessEquipGear(const Protocol::Slot& requestSlot, OUT Protocol::S_EQUIP_GEAR& pkt)
{
    auto* updatedSlotList = pkt.mutable_updated_slots();
    auto* updatedStatList = pkt.mutable_updated_stat();

    if (requestSlot.has_item() == false)
        return false;

    const Protocol::Item& itemInstance = requestSlot.item();
    if (equippedGear->EquipGear(OUT updatedSlotList->Add(), OUT updatedStatList, itemInstance) == false)
        return false;

    if (inventory->removeItem(requestSlot, OUT updatedSlotList->Add()) == false)
        return false;

    // 변경된 스텟 적용
    for (const Protocol::Stat& stat : *updatedStatList)
    {
        SetStatValue(stat.type(), stat.value());
    }

    return true;
}

bool Player::ProcessUnequipGear(const Protocol::Slot& requestSlot, OUT Protocol::S_UNEQUIP_GEAR& pkt)
{
    auto* updatedSlotList = pkt.mutable_updated_slots();
    auto* updatedStatList = pkt.mutable_updated_stat();

    if (requestSlot.has_item() == false)
        return false;

    if (equippedGear->UnequipGear(requestSlot, OUT updatedSlotList->Add(), OUT updatedStatList) == false)
        return false;

    if (inventory->addItem(OUT updatedSlotList->Add(), requestSlot.item()) == false)
        return false;

    // 변경된 스텟 적용
    for (const Protocol::Stat& stat : *updatedStatList)
    {
        SetStatValue(stat.type(), stat.value());
    }

    return true;
}

void Player::OnHit(ObjectRef attacker, Protocol::HitData& hitData)
{
    auto ownerRoom = room.load().lock();
    if (ownerRoom == nullptr)
        return;

    int64 damage = hitData.damage();
    int64 hp = GetStatValue(Protocol::STAT_TYPE_HP);
    int64 updatedHp = hp - damage;

    SetStatValue(Protocol::STAT_TYPE_HP, max(0, updatedHp));

    if (updatedHp > 0)
    {
        // Send Hit Packet
        {
            Protocol::S_HIT hitPkt;

            hitPkt.mutable_hit_data()->CopyFrom(hitData);
            hitPkt.set_hp(updatedHp);

            if (auto ownerSession = session.lock())
            {
                SEND_PACKET_USING_THIS_SESSION(ownerSession, hitPkt);
            }

        }
    }
    else
    {
        // 죽으면 경험치 10% 감소
        int64 exp = GetStatValue(Protocol::STAT_TYPE_EXP);
        int64 maxExp = GetStatValue(Protocol::STAT_TYPE_MAX_EXP);

        int64 lossExp = static_cast<int64>(maxExp * 0.1f);
        int64 curExp = exp;
        int64 updatedExp = curExp <= lossExp ? 0 : curExp - lossExp;

        SetStatValue(Protocol::STAT_TYPE_EXP, updatedExp);

        // Make and Pass On Die Packet
        Protocol::S_DIE DiePkt;
        {
            DiePkt.set_object_id(objectInfo->object_id());
            DiePkt.mutable_die_penalty_details()->set_updated_exp(updatedExp);
        }

        ownerRoom->OnDie(DiePkt);
    }
}

void Player::OnEnterMap(int32 mapId, int32 roomId)
{
    playerInfo->set_map_id(mapId);
    enteringRoomId = roomId;
}

void Player::OnEnterRoom(RoomRef enterRoom, const optional<Protocol::PosInfo>& enterPos)
{
    enteringRoomId = -1;
    room.store(enterRoom);
    playerInfo->set_room_id(enterRoom->GetRoomId());

    if(enterPos.has_value())
    {
        posInfo->CopyFrom(enterPos.value());
    }
    else
    {
        // 만일을 대비한 posInfo 세팅
        const vector3D& centerPos = enterRoom->GetCenterPoint();
        Protocol::Vector* pos = posInfo->mutable_pos();

        pos->set_x(centerPos.x);
        pos->set_y(centerPos.y);
        pos->set_z(centerPos.z);
        posInfo->set_yaw(0.f);
        posInfo->set_state(Protocol::MOVE_STATE_IDLE);
    }
}

void Player::OnMonsterKill(MonsterRef killedMonster, int64 expReward, int64 goldReward)
{
    bool levelUp = false;

    // Get Reward
    {
        int64 updatedExp = GetStatValue(Protocol::STAT_TYPE_EXP) + expReward;
        int64 maxExp = GetStatValue(Protocol::STAT_TYPE_MAX_EXP);
        possession->set_gold(possession->gold() + goldReward);

        // Check Level Up
        if (updatedExp >= maxExp)
        {
            SetStatValue(Protocol::STAT_TYPE_EXP, updatedExp - maxExp);
            OnLevelUp();
            levelUp = true;
        }
    }

    // Send S_MONSTER_KILL_RESULT Packet
    Protocol::S_MONSTER_KILL_RESULT monsterKillResultPkt;
    {
        monsterKillResultPkt.set_object_id(objectInfo->object_id());
        monsterKillResultPkt.set_monster_object_id(killedMonster->objectInfo->object_id());

        monsterKillResultPkt.set_current_exp(GetStatValue(Protocol::STAT_TYPE_EXP));
        monsterKillResultPkt.set_current_gold(possession->gold());

        if (levelUp)
        {
            monsterKillResultPkt.set_is_level_up(true);
            Protocol::LevelUpInfo info;
            info.set_new_level(playerInfo->level() - 1);
            info.set_new_level(playerInfo->level());

            info.set_new_max_hp(GetStatValue(Protocol::STAT_TYPE_MAX_HP));
            info.set_new_max_mp(GetStatValue(Protocol::STAT_TYPE_MAX_MP));
            info.set_new_physical_attack(GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK));
            info.set_new_magical_attack(GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK));
        }
    }

    auto ownerSession = session.lock();
    if (ownerSession == nullptr)
        return;

    SEND_PACKET_USING_THIS_SESSION(ownerSession, monsterKillResultPkt);
}

void Player::OnLevelUp()
{
    // Set Level
    playerInfo->set_level(playerInfo->level() + 1);

    // Set StatInfo
    SetStatValue(Protocol::STAT_TYPE_MAX_EXP, _nextLevelUpData.expRequirement);
    SetStatValue(Protocol::STAT_TYPE_MAX_HP, GetStatValue(Protocol::STAT_TYPE_MAX_HP) + _nextLevelUpData.maxHpIncrement);
    SetStatValue(Protocol::STAT_TYPE_MAX_MP, GetStatValue(Protocol::STAT_TYPE_MAX_MP) + _nextLevelUpData.maxMpIncrement);
    SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK) + _nextLevelUpData.paIncrement);
    SetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK, GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK) + _nextLevelUpData.maIncrement);

    CacheNextLevelUpData();
}

void Player::OnRespawn()
{
    int64 respawnHp = (int64)(GetStatValue(Protocol::STAT_TYPE_MAX_HP) * 0.5f);
    SetStatValue(Protocol::STAT_TYPE_HP, respawnHp);
    idDead = false;
}

void Player::SetStatValue(Protocol::StatType statType, const int64& value)
{
    auto* statMappings = statInfo->mutable_info();
    (*statMappings)[(int32)statType] = value;
}

int64 Player::GetStatValue(Protocol::StatType statType)
{
    auto* statMappings = statInfo->mutable_info();

    return statMappings->at((int32)statType);
}

Protocol::Stat Player::GetStat(Protocol::StatType statType)
{
    int64 value = GetStatValue(statType);
    Protocol::Stat stat;
    {
        stat.set_type(statType);
        stat.set_value(value);
    }

    return stat;
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

        _nextLevelUpData.level = nextLevelData[Level].is_null() ? 0 : static_cast<int32>(nextLevelData[Level]);
        _nextLevelUpData.maxHpIncrement = nextLevelData[MaxHp_Increment].is_null() ? 0 : static_cast<int64>(nextLevelData[MaxHp_Increment]);
        _nextLevelUpData.maxMpIncrement = nextLevelData[MaxMp_Increment].is_null() ? 0 : static_cast<int64>(nextLevelData[MaxMp_Increment]);
        _nextLevelUpData.paIncrement = nextLevelData[PA_Increment].is_null() ? 0 : static_cast<int64>(nextLevelData[PA_Increment]);
        _nextLevelUpData.maIncrement = nextLevelData[MA_Increment].is_null() ? 0 : static_cast<int64>(nextLevelData[MA_Increment]);
        _nextLevelUpData.expRequirement = nextLevelData[ExpRequirement].is_null() ? 0 : static_cast<int64>(nextLevelData[ExpRequirement]);
    }
}
