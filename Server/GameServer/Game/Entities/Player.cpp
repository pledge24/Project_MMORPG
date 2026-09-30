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

    _playerInfo = _entityInfo->mutable_player_info();
    _possession = new Protocol::Possession();
}

Player::~Player()
{
    delete _possession;
}

bool Player::Init()
{
	if (Creature::Init() == false)
		return false;

    _inventory = make_shared<Inventory>(static_pointer_cast<Player>(shared_from_this()));
    _equippedGear = make_shared<EquippedGear>(static_pointer_cast<Player>(shared_from_this()));

	return true;
}

bool Player::Start()
{
	if (Creature::Start() == false)
		return false;

    _inventory->ClearDirtyFlags();
    _equippedGear->ClearDirtyFlag();
    RefreshEquippedGearSummary();

	if (CalculateFinalStat() == false)
		return false;

	CacheNextLevelUpData();

	_respawnRoomMappings[Protocol::RESPAWN_TYPE_TOWN] = RESPAWN_TOWN_ID;
	_respawnRoomMappings[Protocol::RESPAWN_TYPE_CHECKPOINT] = -1;
	_respawnRoomMappings[Protocol::RESPAWN_TYPE_IN_PLACE] = -1;
	_respawnRoomMappings[Protocol::RESPAWN_TYPE_GUILD_BASE] = -1;

	return true;
}

bool Player::ProcessBuyItem(OUT RepeatedPtrField<Protocol::Slot>* updatedSlots, OUT int64& totalGold, int32 templateId, int32 count)
{
    int64 gold = _possession->gold();
    int64 buyPrice = static_cast<int64>(Gamedata::s_itemDataTable[templateId][JsonProperty::Item::BuyPrice]) * count;

    if (gold < buyPrice)
        return false;

    if (_inventory->AddItem(OUT updatedSlots, templateId, count) == false)
        return false;

    totalGold = gold - buyPrice;
    _possession->set_gold(totalGold);

    return true;
}

bool Player::ProcessSellItem(const Protocol::Slot& requestSlot, OUT Protocol::Slot* updatedSlot, OUT int64& totalGold, int32 count)
{
    int64 gold = _possession->gold();
    int32 templateId = requestSlot.item().template_id();
    int64 sellPrice = static_cast<int64>(Gamedata::s_itemDataTable[templateId][JsonProperty::Item::SellPrice]) * count;

    if (_inventory->RemoveItem(requestSlot, OUT updatedSlot, count) == false)
        return false;

    totalGold = gold + sellPrice;
    _possession->set_gold(totalGold);

    return true;
}

bool Player::ProcessUseItem(const Protocol::Slot& requestSlot, OUT Protocol::S_USE_ITEM& pkt)
{
    auto* updatedSlotList = pkt.mutable_updated_slots();
    auto* updatedStatList = pkt.mutable_updated_stat();

    // 아이템 사용으로 인한 슬롯 변경 정보 채우기
    if (_inventory->RemoveItem(requestSlot, OUT updatedSlotList->Add()) == false)
        return false;

    // entityId 채우기
    pkt.set_entity_id(GetEntityId());

    // 변경된 스텟 반영
    for (const Protocol::Stat& stat : pkt.updated_stat())
    {
        int32 templateId = requestSlot.item().template_id();
        const Json& itemData = Gamedata::s_itemDataTable[templateId];

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
    Protocol::Slot* equippedSlot = updatedSlotList->Add();
    if (_equippedGear->EquipGear(OUT equippedSlot, OUT updatedStatList, itemInstance) == false)
        return false;

    // 외형을 바꾸는 쪽은 요청 슬롯(인벤토리 칸)이 아니라 장착된 장비 부위를 알아야 한다.
    pkt.set_slot_id(equippedSlot->slot_id());
    pkt.set_template_id(equippedSlot->item().template_id());
    RefreshEquippedGearSummary();

    if (_inventory->RemoveItem(requestSlot, OUT updatedSlotList->Add()) == false)
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

    Protocol::Slot* unequippedSlot = updatedSlotList->Add();
    if (_equippedGear->UnequipGear(requestSlot, OUT unequippedSlot, OUT updatedStatList) == false)
        return false;

    // 탈착 뒤 그 부위는 비어 있으므로 template_id는 0이다.
    pkt.set_slot_id(unequippedSlot->slot_id());
    pkt.set_template_id(unequippedSlot->item().template_id());
    RefreshEquippedGearSummary();

    if (_inventory->AddItem(OUT updatedSlotList->Add(), requestSlot.item()) == false)
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
	auto ownerRoom = _room.load().lock();
	if (ownerRoom == nullptr)
		return false;

	_posInfo->CopyFrom(*respawnPos);
	{
		pkt.set_success(true);
		pkt.set_respawn_type(type);
		pkt.set_entity_id(GetEntityId());

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
		_isDead = false;
	}

	return true;
}

void Player::OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo)
{
    Creature::OnHit(attacker, attackInfo);

}

void Player::OnDie(EntityRef attacker)
{
    Creature::OnDie(attacker);

}

void Player::OnEnterMap(int32 mapId, int32 roomId)
{
    _playerInfo->set_map_id(mapId);
    _enteringRoomId = roomId;
}

void Player::OnEnterRoom(RoomRef enterRoom, const optional<Protocol::PosInfo>& enterPos)
{
    _enteringRoomId = -1;
    _room.store(enterRoom);
    _playerInfo->set_room_id(enterRoom->GetRoomId());

    if(enterPos.has_value())
    {
        _posInfo->CopyFrom(enterPos.value());
    }
    else
    {
        // 만일을 대비한 _posInfo 세팅
        const vector3D& centerPos = enterRoom->GetCenterPoint();
        Protocol::Vector* pos = _posInfo->mutable_pos();

        pos->set_x(centerPos.x);
        pos->set_y(centerPos.y);
        pos->set_z(centerPos.z);
        _posInfo->set_yaw(0.f);
        _posInfo->set_state(Protocol::MOVE_STATE_IDLE);
    }
}

void Player::OnGetReward(Protocol::S_REWARD_RESULT& rewardResultPkt)
{
    const int32 oldLevel = _playerInfo->level();

    const Protocol::Reward& reward = rewardResultPkt.reward();
    // Get Reward
    {
        _possession->set_gold(_possession->gold() + reward.gold());

        // 경험치가 남는 만큼 여러 레벨을 한 번에 올린다. 레벨이 레벨 표 밖으로 나가면 다음 입장이 막힌다.
        int64 updatedExp = GetStatValue(Protocol::STAT_TYPE_EXP) + reward.exp();
        while (IsMaxLevel() == false && updatedExp >= GetStatValue(Protocol::STAT_TYPE_MAX_EXP))
        {
            updatedExp -= GetStatValue(Protocol::STAT_TYPE_MAX_EXP);
            OnLevelUp();
        }

        // 최대 레벨에서는 경험치를 쌓지 않는다. 남은 경험치와 이후 보상은 버린다.
        SetStatValue(Protocol::STAT_TYPE_EXP, IsMaxLevel() ? 0 : updatedExp);
    }

    // Set Reward Result Pkt
    {
        rewardResultPkt.set_updated_exp(GetStatValue(Protocol::STAT_TYPE_EXP));
        rewardResultPkt.set_updated_gold(_possession->gold());

        if (_playerInfo->level() > oldLevel)
        {
            rewardResultPkt.set_is_level_up(true);
            Protocol::LevelUpInfo* info = rewardResultPkt.mutable_level_up_details();
            info->set_old_level(oldLevel);
            info->set_new_level(_playerInfo->level());

			RepeatedPtrField<Protocol::Stat>* updatedStatList = info->mutable_updated_stat();
			{
				ProtoUtil::AddStat(updatedStatList, Protocol::STAT_TYPE_MAX_HP, GetStatValue(Protocol::STAT_TYPE_MAX_HP));
				ProtoUtil::AddStat(updatedStatList, Protocol::STAT_TYPE_MAX_MP, GetStatValue(Protocol::STAT_TYPE_MAX_MP));
				ProtoUtil::AddStat(updatedStatList, Protocol::STAT_TYPE_PHYSICAL_ATTACK, GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK));
				ProtoUtil::AddStat(updatedStatList, Protocol::STAT_TYPE_MAGICAL_ATTACK, GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK));
			}
        }
    }
}

void Player::RefreshEquippedGearSummary()
{
    // 다른 플레이어는 장비 슬롯을 받지 못하므로, 외형에 필요한 부위와 템플릿만 공개 정보에 싣는다.
    auto* summary = _playerInfo->mutable_equipped_gear_summary();
    summary->clear();

    for (const auto& pair : _possession->equipped_gear())
    {
        const Protocol::Slot& slot = pair.second;
        if (slot.has_item() && slot.item().template_id() != 0)
            (*summary)[slot.slot_id()] = slot.item().template_id();
    }
}

bool Player::IsMaxLevel() const
{
    return _playerInfo->level() >= MAX_LEVEL;
}

void Player::OnLevelUp()
{
    if (IsMaxLevel())
        return;

    // Set Level
    _playerInfo->set_level(_playerInfo->level() + 1);

    // Set StatInfo
    SetStatValue(Protocol::STAT_TYPE_MAX_EXP, _nextLevelUpData.expRequirement);
    SetStatValue(Protocol::STAT_TYPE_MAX_HP, GetStatValue(Protocol::STAT_TYPE_MAX_HP) + _nextLevelUpData.maxHpIncrement);
    SetStatValue(Protocol::STAT_TYPE_MAX_MP, GetStatValue(Protocol::STAT_TYPE_MAX_MP) + _nextLevelUpData.maxMpIncrement);
    SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK) + _nextLevelUpData.paIncrement);
    SetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK, GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK) + _nextLevelUpData.maIncrement);

    CacheNextLevelUpData();
}

void Player::GetRespawnData(Protocol::RespawnType respawnType, OUT RoomRef& respawnRoom, OUT Protocol::PosInfo& respawnPos)
{
    switch (respawnType)
    {
    case Protocol::RESPAWN_TYPE_TOWN:
    case Protocol::RESPAWN_TYPE_CHECKPOINT:
    case Protocol::RESPAWN_TYPE_IN_PLACE:
    case Protocol::RESPAWN_TYPE_GUILD_BASE:
    {
        int32 roomId = GetRespawnRoomId(respawnType);
        respawnRoom = GRoomManager->GetRoomRefFromRoomId(roomId);
        respawnPos = *respawnRoom->GetRespawnPoint();
        break;
    }
    case Protocol::RESPAWN_TYPE_RESURRECTION_ITEM:
    case Protocol::RESPAWN_TYPE_CASH_ITEM:
    {
        // 아이템 사용
        break;
    }
    case Protocol::RESPAWN_TYPE_PARTY_MEMBER:
    case Protocol::RESPAWN_TYPE_BATTLE_RESURRECTION:
    {
        // entityId가 존재하는 경우
        break;
    }

    }
}

PlayerSaveData Player::MakeSaveData() const
{
    PlayerSaveData data;
    data.userId = _userId;
    data.playerInfo.CopyFrom(*_playerInfo);
    data.posInfo.CopyFrom(*_posInfo);
    data.statInfo.CopyFrom(*_statInfo);
    data.possession.CopyFrom(*_possession);

    if (vector<bool>* flags = _inventory->GetDirtyFlags(Protocol::ItemType::ITEM_TYPE_GEAR))
        data.gearDirtyFlags = *flags;
    if (vector<bool>* flags = _inventory->GetDirtyFlags(Protocol::ItemType::ITEM_TYPE_CONSUMABLE))
        data.consumableDirtyFlags = *flags;
    if (vector<bool>* flags = _inventory->GetDirtyFlags(Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS))
        data.miscDirtyFlags = *flags;

    data.equippedGearDirtyFlags = _equippedGear->GetDirtyFlagMappings();

    return data;
}

// 사망한 채 접속이 끊기면 사망 화면에서 마을 리스폰을 누른 것과 같은 상태로 저장한다.
// 사망 여부는 저장되지 않으므로, 그대로 저장하면 다시 접속했을 때 HP 0으로 살아서 들어온다.
bool Player::ApplyTownRespawnForSave()
{
    RoomRef respawnRoom = nullptr;
    Protocol::PosInfo respawnPos;
    GetRespawnData(Protocol::RESPAWN_TYPE_TOWN, OUT respawnRoom, OUT respawnPos);
    if (respawnRoom == nullptr)
        return false;

    respawnPos.set_entity_id(GetEntityId());

    Protocol::S_RESPAWN unusedPkt;
    if (ProcessRespawn(Protocol::RESPAWN_TYPE_TOWN, make_shared<Protocol::PosInfo>(respawnPos), OUT unusedPkt) == false)
        return false;

    _playerInfo->set_room_id(respawnRoom->GetRoomId());
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
    DataTable& classLevelDataTable = (*Gamedata::s_classLevelDataTableMappings[_playerInfo->class_()]);
    int32 level = _playerInfo->level();
    if (classLevelDataTable[level].contains(JsonProperty::LevelTable::MaxHp))
        finalStat.maxHp += static_cast<int32>(classLevelDataTable[level][JsonProperty::LevelTable::MaxHp]);
    if (classLevelDataTable[level].contains(JsonProperty::LevelTable::MaxMp))
        finalStat.maxMp += static_cast<int32>(classLevelDataTable[level][JsonProperty::LevelTable::MaxMp]);
    if (classLevelDataTable[level].contains(JsonProperty::LevelTable::PhysicalAttack))
        finalStat.physical_attack += static_cast<int32>(classLevelDataTable[level][JsonProperty::LevelTable::PhysicalAttack]);
    if (classLevelDataTable[level].contains(JsonProperty::LevelTable::MagicalAttack))
        finalStat.magical_attack += static_cast<int32>(classLevelDataTable[level][JsonProperty::LevelTable::MagicalAttack]);

    // 2. 장착 중이 장비 스텟 추가
    for (const auto& pair : _possession->equipped_gear())
    {
        const Protocol::Item& item = pair.second.item();

        if (item.template_id() == 0)
            continue;

        if (Gamedata::s_itemDataTable[item.template_id()].contains(JsonProperty::Item::Hp))
            finalStat.maxHp += static_cast<int32>(Gamedata::s_itemDataTable[item.template_id()][JsonProperty::Item::Hp]);
        if (Gamedata::s_itemDataTable[item.template_id()].contains(JsonProperty::Item::Mp))
            finalStat.maxMp += static_cast<int32>(Gamedata::s_itemDataTable[item.template_id()][JsonProperty::Item::Mp]);
        if (Gamedata::s_itemDataTable[item.template_id()].contains(JsonProperty::Item::PhysicalAttack))
            finalStat.physical_attack += static_cast<int32>(Gamedata::s_itemDataTable[item.template_id()][JsonProperty::Item::PhysicalAttack]);
        if (Gamedata::s_itemDataTable[item.template_id()].contains(JsonProperty::Item::MagicalAttack))
            finalStat.magical_attack += static_cast<int32>(Gamedata::s_itemDataTable[item.template_id()][JsonProperty::Item::MagicalAttack]);
    }

    // validate
    try
    {
        if (GetStatValue(Protocol::STAT_TYPE_HP) > finalStat.maxHp)
            throw wstring(L"현재 HP가 최대 HP를 초과");
        if (GetStatValue(Protocol::STAT_TYPE_MP) > finalStat.maxMp)
            throw wstring(L"현재 MP가 최대 MP를 초과");
        if (GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK) != finalStat.physical_attack)
            throw wstring(L"물리 공격력이 계산 결과와 일치하지 않음");
        if (GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK) != finalStat.magical_attack)
            throw wstring(L"마법 공격력이 계산 결과와 일치하지 않음");
    }
    catch (wstring& cause)
    {
        wcerr << L"스텟 계산에 문제가 생겼습니다. 사유: " << cause << endl;
        return false;
    }

    // Protocol::statInfo에 최종 스텟 적용
    SetStatValue(Protocol::STAT_TYPE_MAX_HP, finalStat.maxHp);
    SetStatValue(Protocol::STAT_TYPE_MAX_MP, finalStat.maxMp);
    SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, finalStat.physical_attack);
    SetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK, finalStat.magical_attack);

    return true;
}

void Player::CacheNextLevelUpData()
{
    int32 nextLevel = _playerInfo->level() + 1;
    if (nextLevel > (int32)MAX_LEVEL)
    {
        cout << "Current Level is Max! Can't Cache Level Up Data" << '\n';
        return;
    }

    DataTable& classLevelDataTable = (*Gamedata::s_classLevelDataTableMappings[_playerInfo->class_()]);
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
