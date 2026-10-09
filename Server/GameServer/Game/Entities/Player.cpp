#include "Core/pch.h"
#include "Game/Entities/Player.h"
#include "Game/Inventory/Inventory.h"
#include "Game/Equipment/EquippedGear.h"
#include "Game/Room/Room.h"

namespace
{
    // 아이템 요청은 서버 슬롯에 든 아이템으로 판정한다. 요청에 실린 아이템은 클라이언트 슬롯이
    // 어긋났는지 대조하는 데만 쓴다. 서버 아이템에 uid가 있으면 uid까지 같아야 같은 아이템이다.
    bool MatchesRequest(const Protocol::Slot* ownedSlot, const Protocol::Slot& requestSlot)
    {
        if (ownedSlot == nullptr || ownedSlot->has_item() == false || requestSlot.has_item() == false)
            return false;

        const Protocol::Item& owned = ownedSlot->item();
        const Protocol::Item& requested = requestSlot.item();
        if (owned.template_id() != requested.template_id())
            return false;

        if (owned.has_item_uid() && owned.item_uid() != requested.item_uid())
            return false;

        return true;
    }
}

Player::Player()
{
	_isPlayer = true;
    _isTickable = false;

    _entityInfo->set_entity_type(Protocol::EntityType::ENTITY_TYPE_PLAYER);
    _playerInfo = _entityInfo->mutable_player_info();
    _possession = new Protocol::Possession();
}

Player::~Player()
{
    delete _possession;
}

bool Player::OnLoaded()
{
    _inventory->ClearDirtyFlags();
    _equippedGear->ClearDirtyFlag();
    RefreshEquippedGearSummary();

	if (CalculateFinalStat() == false)
		return false;

	CacheNextLevelUpData();

	return true;
}

bool Player::Init(const SpawnParams& params)
{
	if (Creature::Init(params) == false)
		return false;

    PlayerRef self = static_pointer_cast<Player>(shared_from_this());

    // 세션 연결을 인벤토리 생성보다 먼저 한다. 팩토리가 nullptr를 돌려줘도 세션의 _player는 이미
    // 바뀌어 있는 동작을 옮겨 온 것이다(TD-019).
    if (params.session != nullptr)
    {
        _session = params.session;
        _userId = params.session->_userId;
        params.session->_player.store(self);
    }

    _inventory = make_shared<Inventory>(self);
    _equippedGear = make_shared<EquippedGear>(self);

	return true;
}

bool Player::ProcessBuyItem(OUT RepeatedPtrField<Protocol::Slot>* updatedSlots, OUT int64& totalGold, int32 templateId, int32 count)
{
    const ItemTemplate* itemTemplate = Gamedata::FindItem(templateId);
    if (itemTemplate == nullptr)
        return false;

    int64 gold = _possession->gold();
    int64 buyPrice = itemTemplate->buyPrice * count;

    if (gold < buyPrice)
        return false;

    // 실제 "아이템 구매" 적용 시점.
    if (_inventory->AddItem(OUT updatedSlots, templateId, count) == false)
        return false;

    totalGold = gold - buyPrice;
    _possession->set_gold(totalGold);

    return true;
}

bool Player::ProcessSellItem(const Protocol::Slot& requestSlot, OUT Protocol::Slot* updatedSlot, OUT int64& totalGold, int32 count)
{
    // 가격은 요청이 아니라 슬롯에 든 아이템으로 정한다.
    const Protocol::Slot* ownedSlot = _inventory->GetSlot(requestSlot.type(), requestSlot.slot_id());
    if (MatchesRequest(ownedSlot, requestSlot) == false)
        return false;

    const ItemTemplate* itemTemplate = Gamedata::FindItem(ownedSlot->item().template_id());
    if (itemTemplate == nullptr || itemTemplate->sellable == false)
        return false;

    int64 gold = _possession->gold();
    int64 sellPrice = itemTemplate->sellPrice * count;

    // 실제 "아이템 판매" 적용 시점.
    if (_inventory->RemoveItem(requestSlot, OUT updatedSlot, count) == false)
        return false;

    totalGold = gold + sellPrice;
    _possession->set_gold(totalGold);

    return true;
}

bool Player::ProcessUseItem(const Protocol::Slot& requestSlot, uint64 nowMs, OUT Protocol::S_USE_ITEM& pkt)
{
    // 거부 응답에도 싣는다. 클라이언트는 이 id로 내 플레이어를 찾은 뒤에야 요청 대기를 푼다.
    pkt.set_entity_id(GetEntityId());

    if (IsDead())
        return false;

    if (requestSlot.type() != Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE)
        return false;

    // 효과는 요청이 아니라 슬롯에 든 아이템으로 정한다. RemoveItem이 마지막 한 개를 지우므로 그 전에 읽는다.
    const Protocol::Slot* ownedSlot = _inventory->GetSlot(requestSlot.type(), requestSlot.slot_id());
    if (MatchesRequest(ownedSlot, requestSlot) == false)
        return false;

    const int32 templateId = ownedSlot->item().template_id();
    const ItemTemplate* itemTemplate = Gamedata::FindItem(templateId);
    if (itemTemplate == nullptr)
        return false;

    // 재사용 대기는 템플릿마다 따로 돈다.
    auto lastUseIt = _lastUseTimeMs.find(templateId);
    if (lastUseIt != _lastUseTimeMs.end() && nowMs < lastUseIt->second + itemTemplate->cooldownMs)
        return false;

    // 제거에 성공했을 때만 응답에 슬롯을 싣는다. 거부 응답에는 슬롯이 없다.
    Protocol::Slot updatedSlot;
    if (_inventory->RemoveItem(requestSlot, OUT &updatedSlot) == false)
        return false;

    *pkt.mutable_updated_slots()->Add() = std::move(updatedSlot);

    _lastUseTimeMs[templateId] = nowMs;

    // 회복률이 있는 스탯만 바꾸고 싣는다. 이미 가득 차 있어도 소비하고 값은 최대로 둔다.
    auto restore = [&](double ratio, Protocol::StatType maxType, Protocol::StatType curType)
        {
            if (ratio <= 0)
                return;

            const int64 maxValue = GetStatValue(maxType);
            const int64 amount = static_cast<int64>(maxValue * ratio);
            const int64 updatedValue = min(maxValue, GetStatValue(curType) + amount);
            SetStatValue(curType, updatedValue);

            Protocol::Stat* updatedStat = pkt.mutable_updated_stat()->Add();
            updatedStat->set_type(curType);
            updatedStat->set_value(updatedValue);
        };

    restore(itemTemplate->hpRestoreRatio, Protocol::STAT_TYPE_MAX_HP, Protocol::STAT_TYPE_HP);
    restore(itemTemplate->mpRestoreRatio, Protocol::STAT_TYPE_MAX_MP, Protocol::STAT_TYPE_MP);

    return true;
}

bool Player::ProcessEquipGear(const Protocol::Slot& requestSlot, OUT Protocol::S_EQUIP_GEAR& pkt)
{
    auto* updatedSlotList = pkt.mutable_updated_slots();
    auto* updatedStatList = pkt.mutable_updated_stat();

    // 입는 것은 요청이 아니라 인벤토리 슬롯에 든 장비다.
    const Protocol::Slot* ownedSlot = _inventory->GetSlot(requestSlot.type(), requestSlot.slot_id());
    if (MatchesRequest(ownedSlot, requestSlot) == false || ownedSlot->item().count() < 1)
        return false;

    // 착용이 실패하면 아무것도 바뀌지 않는다. 착용이 성공하면 위에서 확인한 슬롯이라 제거는 실패하지 않는다.
    const Protocol::Item ownedItem = ownedSlot->item();
    Protocol::Slot* equippedSlot = updatedSlotList->Add();
    if (_equippedGear->EquipGear(OUT equippedSlot, OUT updatedStatList, ownedItem) == false)
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

    // 돌려받는 것은 요청이 아니라 장비 칸에 든 장비다.
    if (requestSlot.type() != Protocol::SLOT_TYPE_EQUIPPED)
        return false;

    const Protocol::Slot* ownedSlot = _equippedGear->GetSlot(requestSlot.slot_id());
    if (MatchesRequest(ownedSlot, requestSlot) == false)
        return false;

    // 넣을 자리가 없으면 장비 칸을 비우기 전에 거절한다. 비운 뒤에 실패하면 장비가 사라진다.
    const Protocol::Item ownedItem = ownedSlot->item();
    if (_inventory->FindFirstAvailableSlotId(Protocol::ItemType::ITEM_TYPE_GEAR, ownedItem.template_id()) == -1)
        return false;

    Protocol::Slot* unequippedSlot = updatedSlotList->Add();
    if (_equippedGear->UnequipGear(requestSlot.slot_id(), OUT unequippedSlot, OUT updatedStatList) == false)
        return false;

    // 탈착 뒤 그 부위는 비어 있으므로 template_id는 0이다.
    pkt.set_slot_id(unequippedSlot->slot_id());
    pkt.set_template_id(unequippedSlot->item().template_id());
    RefreshEquippedGearSummary();

    if (_inventory->AddItem(OUT updatedSlotList->Add(), ownedItem) == false)
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
        // 레벨 표에 다음 레벨 행이 없으면 maxExp가 0이 된다. 그때 레벨을 올리면 보상 한 번에 최대 레벨까지 간다.
        int64 updatedExp = GetStatValue(Protocol::STAT_TYPE_EXP) + reward.exp();
        while (IsMaxLevel() == false)
        {
            const int64 maxExp = GetStatValue(Protocol::STAT_TYPE_MAX_EXP);
            if (maxExp <= 0 || updatedExp < maxExp)
                break;

            updatedExp -= maxExp;
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
				// 클라이언트에는 레벨 표가 없어 새 최대 경험치를 이 패킷으로만 안다.
				ProtoUtil::AddStat(updatedStatList, Protocol::STAT_TYPE_MAX_EXP, GetStatValue(Protocol::STAT_TYPE_MAX_EXP));
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

bool Player::IsMaxLevel() const
{
    // 레벨 표가 없는 직업은 오를 레벨이 없으므로 최대 레벨로 본다.
    const ClassLevelTable* classLevelTable = Gamedata::FindClassLevelTable(_playerInfo->class_());
    return classLevelTable == nullptr || _playerInfo->level() >= classLevelTable->GetMaxLevel();
}

bool Player::FindTownRespawnPoint(OUT RoomRef& respawnRoom, OUT Protocol::PosInfo& respawnPos)
{
    RoomRef townRoom = GRoomManager->GetRoomRefFromRoomId(Gamedata::GetTownRoomId());
    if (townRoom == nullptr)
        return false;

    shared_ptr<Protocol::PosInfo> respawnPoint = townRoom->GetRespawnPoint();
    if (respawnPoint == nullptr)
        return false;

    respawnRoom = townRoom;
    respawnPos = *respawnPoint;
    return true;
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
    if (FindTownRespawnPoint(OUT respawnRoom, OUT respawnPos) == false)
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
    const ClassLevelTable* classLevelTable = Gamedata::FindClassLevelTable(_playerInfo->class_());
    if (classLevelTable == nullptr)
    {
        GLogger->Error("스텟 계산에 문제가 생겼습니다. 사유: 레벨 표에 없는 직업 {}", static_cast<int32>(_playerInfo->class_()));
        return false;
    }

    const LevelTemplate* levelTemplate = classLevelTable->Find(_playerInfo->level());
    if (levelTemplate == nullptr)
    {
        GLogger->Error("스텟 계산에 문제가 생겼습니다. 사유: 레벨 표에 없는 레벨 {}", _playerInfo->level());
        return false;
    }

    finalStat.maxHp += levelTemplate->maxHp;
    finalStat.maxMp += levelTemplate->maxMp;
    finalStat.physical_attack += levelTemplate->physicalAttack;
    finalStat.magical_attack += levelTemplate->magicalAttack;

    // 2. 장착 중이 장비 스텟 추가
    for (const auto& pair : _possession->equipped_gear())
    {
        const Protocol::Item& item = pair.second.item();

        if (item.template_id() == 0)
            continue;

        // 장비 칸에는 표에 있는 아이템만 들어간다(EquipGear가 거른다).
        const ItemTemplate* itemTemplate = Gamedata::FindItem(item.template_id());
        if (itemTemplate == nullptr)
            continue;

        finalStat.maxHp += itemTemplate->hp;
        finalStat.maxMp += itemTemplate->mp;
        finalStat.physical_attack += itemTemplate->physicalAttack;
        finalStat.magical_attack += itemTemplate->magicalAttack;
    }

    // validate
    try
    {
        if (GetStatValue(Protocol::STAT_TYPE_HP) > finalStat.maxHp)
            throw string("현재 HP가 최대 HP를 초과");
        if (GetStatValue(Protocol::STAT_TYPE_MP) > finalStat.maxMp)
            throw string("현재 MP가 최대 MP를 초과");
        if (GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK) != finalStat.physical_attack)
            throw string("물리 공격력이 계산 결과와 일치하지 않음");
        if (GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK) != finalStat.magical_attack)
            throw string("마법 공격력이 계산 결과와 일치하지 않음");
    }
    catch (const string& cause)
    {
        GLogger->Error("스텟 계산에 문제가 생겼습니다. 사유: {}", cause);
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
    // 최대 레벨에서는 오를 레벨이 없으므로 캐시를 그대로 둔다.
    if (IsMaxLevel())
        return;

    // 다음 레벨 행이 없으면 빈 값으로 둔다.
    _nextLevelUpData = NextLevelUpData{};

    const LevelTemplate* nextLevelTemplate = Gamedata::FindClassLevelTable(_playerInfo->class_())->Find(_playerInfo->level() + 1);
    if (nextLevelTemplate == nullptr)
        return;

    _nextLevelUpData.level = nextLevelTemplate->level;
    _nextLevelUpData.maxHpIncrement = nextLevelTemplate->maxHpIncrement;
    _nextLevelUpData.maxMpIncrement = nextLevelTemplate->maxMpIncrement;
    _nextLevelUpData.paIncrement = nextLevelTemplate->paIncrement;
    _nextLevelUpData.maIncrement = nextLevelTemplate->maIncrement;
    _nextLevelUpData.expRequirement = nextLevelTemplate->expRequirement;
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
