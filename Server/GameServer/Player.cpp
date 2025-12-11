#include "pch.h"
#include "Player.h"
#include "Inventory.h"
#include "EquippedGear.h"
#include "Monster.h"
#include "Room.h"

Player::Player()
{
	_isPlayer = true;
    _isTickable = false;

    playerInfo = objectInfo->mutable_player_info();
    possession = new Protocol::Possession();
}

Player::~Player()
{
    delete possession;
}

bool Player::Init(Protocol::PosInfo* spawnPos)
{
	if (Creature::Init(spawnPos) == false)
		return false;

    inventory = make_shared<Inventory>(static_pointer_cast<Player>(shared_from_this()));
    equippedGear = make_shared<EquippedGear>(static_pointer_cast<Player>(shared_from_this()));

	return true;
}

bool Player::Start()
{
	if (Creature::Start() == false)
		return false;

	if (CalculateFinalStat() == false)
		return false;

	CacheNextLevelUpData();

	respawnRoomMappings[Protocol::RESPAWN_TYPE_TOWN] = RESPAWN_TOWN_ID;
	respawnRoomMappings[Protocol::RESPAWN_TYPE_CHECKPOINT] = -1;
	respawnRoomMappings[Protocol::RESPAWN_TYPE_IN_PLACE] = -1;
	respawnRoomMappings[Protocol::RESPAWN_TYPE_GUILD_BASE] = -1;

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

bool Player::ProcessRespawn(Protocol::RespawnType type, shared_ptr<Protocol::PosInfo> respawnPos, OUT Protocol::S_RESPAWN& pkt)
{
	auto ownerRoom = room.load().lock();
	if (ownerRoom == nullptr)
		return false;

	posInfo->CopyFrom(*respawnPos);
	{
		pkt.set_success(true);
		pkt.set_respawn_type(type);
		pkt.set_object_id(objectInfo->object_id());

		pkt.set_room_id(ownerRoom->GetRoomId());
		pkt.mutable_pos_info()->CopyFrom(*respawnPos);
	}

	switch (type)
	{
	case Protocol::RESPAWN_TYPE_TOWN:
	{
		RepeatedPtrField<Protocol::Stat>* updatedStatList = pkt.mutable_updated_stat();

		// 사망 패널티 적용(경험치 10% 감소) <- 리스폰 할때 적용
		{
			int64 exp = GetStatValue(Protocol::STAT_TYPE_EXP);
			int64 maxExp = GetStatValue(Protocol::STAT_TYPE_MAX_EXP);

			int64 lossExp = static_cast<int64>(maxExp * 0.1f);
			int64 curExp = exp;
			int64 updatedExp = curExp <= lossExp ? 0 : curExp - lossExp;

			SetStatValue(Protocol::STAT_TYPE_EXP, updatedExp);
			ProtoUtil::AddStat(updatedStatList, Protocol::STAT_TYPE_EXP, updatedExp);
		}

		// Hp는 절반만 가지고 리스폰
		{
			int64 respawnHp = (int64)(GetStatValue(Protocol::STAT_TYPE_MAX_HP) * 0.5f);

			SetStatValue(Protocol::STAT_TYPE_HP, respawnHp);
			ProtoUtil::AddStat(updatedStatList, Protocol::STAT_TYPE_HP, respawnHp);
		}

		break;
	}
	case Protocol::RESPAWN_TYPE_CHECKPOINT:
	case Protocol::RESPAWN_TYPE_RESURRECTION_ITEM:
	case Protocol::RESPAWN_TYPE_IN_PLACE:
	case Protocol::RESPAWN_TYPE_PARTY_MEMBER:
	case Protocol::RESPAWN_TYPE_GUILD_BASE:
	case Protocol::RESPAWN_TYPE_CASH_ITEM:
	case Protocol::RESPAWN_TYPE_BATTLE_RESURRECTION:
	{
		break;
	}
	default:
		break;
	}

	// 리스폰 성공 처리
	{
		isDead = false;
	}

	return true;
}

void Player::OnHit(ObjectRef attacker, Protocol::AttackInfo attackInfo)
{
    Creature::OnHit(attacker, attackInfo);

}

void Player::OnDie(ObjectRef attacker)
{
    Creature::OnDie(attacker);

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

void Player::OnGetReward(Protocol::S_REWARD_RESULT& rewardResultPkt)
{
    bool levelUp = false;

    const Protocol::Reward& reward = rewardResultPkt.reward();
    // Get Reward
    {
        int64 updatedExp = GetStatValue(Protocol::STAT_TYPE_EXP) + reward.exp();
        int64 maxExp = GetStatValue(Protocol::STAT_TYPE_MAX_EXP);
        possession->set_gold(possession->gold() + reward.gold());

        // Check Level Up
        if (updatedExp >= maxExp)
        {
            SetStatValue(Protocol::STAT_TYPE_EXP, updatedExp - maxExp);
            OnLevelUp();
            levelUp = true;
        }
    }

    // Set Reward Result Pkt
    {
        rewardResultPkt.set_updated_exp(GetStatValue(Protocol::STAT_TYPE_EXP));
        rewardResultPkt.set_updated_gold(possession->gold());

        if (levelUp)
        {
            rewardResultPkt.set_is_level_up(true);
            Protocol::LevelUpInfo info;
            info.set_new_level(playerInfo->level() - 1);
            info.set_new_level(playerInfo->level());

			RepeatedPtrField<Protocol::Stat>* updatedStatList = info.mutable_updated_stat();
			{
				ProtoUtil::AddStat(updatedStatList, Protocol::STAT_TYPE_MAX_HP, GetStatValue(Protocol::STAT_TYPE_MAX_HP));
				ProtoUtil::AddStat(updatedStatList, Protocol::STAT_TYPE_MAX_MP, GetStatValue(Protocol::STAT_TYPE_MAX_MP));
				ProtoUtil::AddStat(updatedStatList, Protocol::STAT_TYPE_PHYSICAL_ATTACK, GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK));
				ProtoUtil::AddStat(updatedStatList, Protocol::STAT_TYPE_MAGICAL_ATTACK, GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK));
			}
        }
    }
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
    if (classLevelDataTable[level].contains(JsonProperty::LevelTable::MaxHp))
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
        if (statMappings->at((int32)Protocol::STAT_TYPE_PHYSICAL_ATTACK) != finalStat.physical_attack)
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
