#include "Core/pch.h"
#include "Game/Entities/Player.h"
#include "Game/Inventory/InventoryComponent.h"
#include "Game/Equipment/EquipmentComponent.h"
#include "Game/QuickSlots/QuickSlotComponent.h"
#include "Game/Progress/Leveling.h"
#include "Game/Room/Room.h"

namespace
{
    // 아이템 요청은 서버의 칸에 든 아이템으로 판정한다. 요청의 uid는 클라이언트의 칸이 어긋났는지
    // 대조하는 데만 쓴다. 칸에 놓인 아이템은 언제나 uid가 있으므로 uid가 0인 요청은 빈 칸을 본 것이다.
    bool MatchesRequest(const ItemInstance* owned, const ItemSlotRequest& request)
    {
        if (owned == nullptr || request.itemUid == 0)
            return false;

        return owned->itemUid == request.itemUid;
    }
}

Player::Player()
{
	_isPlayer = true;

    _playerInfo = _entityInfo->mutable_player_info();
}

Player::~Player()
{
}

bool Player::Init(const SpawnParams& params)
{
	if (Creature::Init(params) == false)
		return false;

    PlayerRef self = static_pointer_cast<Player>(shared_from_this());

    _session = params.session;
    _userId = params.userId;

    _inventory = make_shared<InventoryComponent>(self);
    _equipment = make_shared<EquipmentComponent>(self);
    _quickSlots = make_shared<QuickSlotComponent>(self);

    if (params.progress != nullptr)
        return ApplyProgress(*params.progress) && OnLoaded();

	return true;
}

void Player::Start()
{
    Creature::Start();

    // 컴포넌트들 시작.
    _inventory->Start();
    _equipment->Start();
    _quickSlots->Start();
}

void Player::Tick(float deltaTime)
{
    Creature::Tick(deltaTime);

    // 컴포넌트들 Tick 처리.
    _inventory->Tick(deltaTime);
    _equipment->Tick(deltaTime);
    _quickSlots->Tick(deltaTime);
}

bool Player::OnLoaded()
{
    RefreshEquippedGearSummary();

	if (ValidateLoadedStat() == false)
		return false;

	RefreshFinalStat(nullptr);

	CacheNextLevelUpData();

	return true;
}

bool Player::ApplyProgress(const PlayerProgress& progress)
{
    const int64 characterId = progress.characterId;
    _characterId = characterId;
    _characterClass = static_cast<Protocol::CharacterClass>(progress.classId);
    _playerInfo->set_name(progress.name);
    _playerInfo->set_level(progress.level);

    _mapId = progress.mapId;

    Protocol::PosInfo spawnPosInfo;
    spawnPosInfo.mutable_pos()->set_x(progress.pos.x);
    spawnPosInfo.mutable_pos()->set_y(progress.pos.y);
    spawnPosInfo.mutable_pos()->set_z(progress.pos.z);
    spawnPosInfo.set_yaw(progress.yaw);
    spawnPosInfo.set_state(Protocol::MOVE_STATE_IDLE);
    SetPosInfo(spawnPosInfo);

    SetStatValue(Protocol::STAT_TYPE_HP, progress.hp);
    SetStatValue(Protocol::STAT_TYPE_MP, progress.mp);

    // DB의 경험치는 누적값이고 레벨은 캐릭터 목록에 보일 사본이다. 레벨은 누적 경험치와 레벨 표로 다시 정한다.
    // 레벨 표가 없는 직업은 OnLoaded의 스탯 계산이 거절한다.
    if (const ClassLevelTable* classLevelTable = Gamedata::FindClassLevelTable(_characterClass))
    {
        const int64 loadedExp = progress.totalExp;
        const int64 clampedExp = Leveling::ClampTotalExp(*classLevelTable, loadedExp);
        if (clampedExp != loadedExp)
            GLogger->Warning("캐릭터 {}의 누적 경험치 {}가 범위를 벗어나 {}로 자릅니다", characterId, loadedExp, clampedExp);

        const LevelExp levelExp = SetTotalExp(*classLevelTable, clampedExp);
        if (levelExp.level != progress.level)
            GLogger->Info("캐릭터 {}의 저장된 레벨 {}이 누적 경험치로 계산한 레벨 {}과 달라 계산값을 씁니다", characterId, progress.level, levelExp.level);

        _playerInfo->set_level(levelExp.level);
    }

    _gold = progress.gold;

    // 잘못된 행 하나를 건너뛰고 입장시키면, 그 칸에 다른 아이템이 들어가 저장될 때 그 행을 덮어쓴다.
    // 그래서 입장을 거절하고 행은 DB에 남긴다.
    for (const PlacedItem& placed : progress.items)
    {
        const SlotAddress& address = placed.address;
        const bool loaded = address.container == ContainerType::EQUIPPED
            ? _equipment->LoadEquipped(placed.item, address.slotIndex)
            : _inventory->LoadItem(address, placed.item);

        if (loaded == false)
        {
            GLogger->Error("캐릭터 {}의 아이템(컨테이너 {}, 칸 {}, 템플릿 {})을 넣을 수 없습니다",
                characterId, static_cast<int32>(address.container), address.slotIndex, placed.item.templateId);
            return false;
        }
    }

    // 불러온 칸은 응답에 실을 변경이 아니다. 남겨 두면 첫 요청의 응답에 함께 실린다.
    TakeItemChanges();

    // 퀵슬롯은 아이템을 담지 않으므로 버려도 잃는 것이 없다. 넣을 수 없는 등록은 건너뛰고 입장시키고, 다음 저장이 그 행을 지운다.
    // 기획에서 퀵슬롯 개수를 줄이거나 아이템 종류를 바꾸면 생긴다.
    for (const QuickSlot& quickSlot : progress.quickSlots)
    {
        if (_quickSlots->Load(quickSlot) == false)
        {
            const QuickSlotRef ref = quickSlot.ref.value_or(QuickSlotRef{});
            GLogger->Warning("캐릭터 {}의 퀵슬롯 {}(종류 {}, 대상 {})을 등록할 수 없어 버립니다", characterId, quickSlot.slotIndex,
                static_cast<int32>(ref.type), ref.id);
        }
    }

    return true;
}

bool Player::ValidateLoadedStat()
{
    const optional<CombatStats> finalStat = CalculateFinalStat();
    if (finalStat.has_value() == false)
        return false;

    // 공격력은 저장하지 않고 레벨 표와 착용 장비로 다시 계산하므로 대조할 값이 없다.
    string cause;
    if (GetStatValue(Protocol::STAT_TYPE_HP) > finalStat->maxHp)
        cause = "현재 HP가 최대 HP를 초과";
    else if (GetStatValue(Protocol::STAT_TYPE_MP) > finalStat->maxMp)
        cause = "현재 MP가 최대 MP를 초과";

    if (cause.empty() == false)
    {
        GLogger->Error("스텟 계산에 문제가 생겼습니다. 사유: {}", cause);
        return false;
    }

    return true;
}

optional<BuyItemResult> Player::ProcessBuyItem(int32 templateId, int32 count)
{
    const ItemTemplate* itemTemplate = Gamedata::FindItem(templateId);
    if (itemTemplate == nullptr)
        return nullopt;

    const int64 buyPrice = itemTemplate->buyPrice * count;

    if (_gold < buyPrice)
        return nullopt;

    if (_inventory->AddNewItem(templateId, count) == false)
        return nullopt;

    _gold -= buyPrice;

    BuyItemResult result;
    result.changedSlots = TakeItemChanges();
    result.gold = _gold;

    return result;
}

optional<SellItemResult> Player::ProcessSellItem(const ItemSlotRequest& request, int32 count)
{
    // 가격은 요청이 아니라 칸에 든 아이템으로 정한다.
    const ItemInstance* owned = _inventory->FindItem(request.address);
    if (MatchesRequest(owned, request) == false)
        return nullopt;

    const ItemTemplate* itemTemplate = Gamedata::FindItem(owned->templateId);
    if (itemTemplate == nullptr || itemTemplate->sellable == false)
        return nullopt;

    const int64 sellPrice = itemTemplate->sellPrice * count;

    if (_inventory->RemoveItem(request.address, count) == false)
        return nullopt;

    _gold += sellPrice;

    SellItemResult result;
    result.changedSlots = TakeItemChanges();
    result.gold = _gold;

    return result;
}

optional<UseItemResult> Player::ProcessUseItem(const ItemSlotRequest& request, uint64 nowMs)
{
    if (IsDead())
        return nullopt;

    if (request.address.container != ContainerType::INVENTORY_CONSUMABLE)
        return nullopt;

    // 효과는 요청이 아니라 칸에 든 아이템으로 정한다. RemoveItem이 마지막 한 개를 지우므로 그 전에 읽는다.
    const ItemInstance* owned = _inventory->FindItem(request.address);
    if (MatchesRequest(owned, request) == false)
        return nullopt;

    const int32 templateId = owned->templateId;
    const ItemTemplate* itemTemplate = Gamedata::FindItem(templateId);
    if (itemTemplate == nullptr)
        return nullopt;

    if (_inventory->IsCoolingDown(templateId, nowMs))
        return nullopt;

    if (_inventory->RemoveItem(request.address) == false)
        return nullopt;

    _inventory->StartCooldown(templateId, nowMs);

    UseItemResult result;
    result.changedSlots = TakeItemChanges();

    // 회복률이 있는 스탯만 바꾸고 싣는다. 이미 가득 차 있어도 소비하고 값은 최대로 둔다.
    auto restore = [&](double ratio, Protocol::StatType maxType, Protocol::StatType curType)
        {
            if (ratio <= 0)
                return;

            const int64 maxValue = GetStatValue(maxType);
            const int64 amount = static_cast<int64>(maxValue * ratio);
            const int64 updatedValue = min(maxValue, GetStatValue(curType) + amount);
            SetStatValue(curType, updatedValue);

            ProtoUtil::AddStat(&result.updatedStats, curType, updatedValue);
        };

    restore(itemTemplate->hpRestoreRatio, Protocol::STAT_TYPE_MAX_HP, Protocol::STAT_TYPE_HP);
    restore(itemTemplate->mpRestoreRatio, Protocol::STAT_TYPE_MAX_MP, Protocol::STAT_TYPE_MP);

    return result;
}

optional<GearChangeResult> Player::ProcessEquipGear(const ItemSlotRequest& request)
{
    // 입는 것은 요청이 아니라 인벤토리 칸에 든 장비다.
    const ItemInstance* owned = _inventory->FindItem(request.address);
    if (MatchesRequest(owned, request) == false)
        return nullopt;

    // 착용이 실패하면 아무것도 바뀌지 않는다. 착용이 성공하면 위에서 확인한 칸이라 제거는 실패하지 않는다.
    const ItemInstance ownedItem = *owned;
    const optional<int32> gearType = _equipment->Equip(ownedItem);
    if (gearType.has_value() == false)
        return nullopt;

    // 외형을 바꾸는 쪽은 요청한 칸(인벤토리 칸)이 아니라 장착된 장비 부위를 알아야 한다.
    GearChangeResult result;
    result.gearType = gearType.value();
    result.templateId = ownedItem.templateId;
    RefreshEquippedGearSummary();

    if (_inventory->RemoveItem(request.address) == false)
        return nullopt;

    result.changedSlots = TakeItemChanges();
    RefreshFinalStat(OUT &result.updatedStats);

    return result;
}

optional<GearChangeResult> Player::ProcessUnequipGear(const ItemSlotRequest& request)
{
    // 돌려받는 것은 요청이 아니라 장비 칸에 든 장비다.
    if (request.address.container != ContainerType::EQUIPPED)
        return nullopt;

    const int32 gearType = request.address.slotIndex;
    const ItemInstance* owned = _equipment->FindItem(gearType);
    if (MatchesRequest(owned, request) == false)
        return nullopt;

    // 넣을 자리가 없으면 장비 칸을 비우기 전에 거절한다. 비운 뒤에 실패하면 장비가 사라진다.
    if (_inventory->CanAddItem(*owned) == false)
        return nullopt;

    optional<ItemInstance> unequipped = _equipment->Unequip(gearType);
    if (unequipped.has_value() == false)
        return nullopt;

    // 탈착 뒤 그 부위는 비어 있으므로 templateId는 0이다.
    GearChangeResult result;
    result.gearType = gearType;
    result.templateId = 0;
    RefreshEquippedGearSummary();

    if (_inventory->AddItem(unequipped.value()) == false)
        return nullopt;

    result.changedSlots = TakeItemChanges();
    RefreshFinalStat(OUT &result.updatedStats);

    return result;
}

optional<SetQuickSlotResult> Player::ProcessSetQuickSlot(int32 slotIndex, const optional<QuickSlotRef>& ref)
{
    optional<vector<QuickSlot>> changed = _quickSlots->Set(slotIndex, ref);
    if (changed.has_value() == false)
        return nullopt;

    return SetQuickSlotResult{ std::move(changed.value()) };
}

optional<UseItemResult> Player::ProcessUseQuickSlot(int32 slotIndex, uint64 nowMs)
{
    // 퀵슬롯은 아이템 정의를 가리키므로 쓸 칸은 서버가 고른다. 사용 규칙은 칸을 골라 쓰는 사용과 한 곳에 둔다.
    const optional<QuickSlotRef> ref = _quickSlots->Find(slotIndex);
    if (ref.has_value() == false || ref->type != QuickSlotRefType::ITEM)
        return nullopt;

    const ItemContainer* consumables = GetInventory().FindSection(ContainerType::INVENTORY_CONSUMABLE);
    if (consumables == nullptr)
        return nullopt;

    // 여러 칸에 나뉘어 있으면 칸 번호가 가장 작은 묶음부터 쓴다.
    const optional<int32> chosenSlot = consumables->FindFirstSlotOf(ref->id);
    if (chosenSlot.has_value() == false)
        return nullopt;

    ItemSlotRequest request;
    request.address = SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, chosenSlot.value() };
    request.itemUid = consumables->Find(chosenSlot.value())->itemUid;
    return ProcessUseItem(request, nowMs);
}

RespawnResult Player::ProcessRespawn(Protocol::RespawnType type, const Protocol::PosInfo& respawnPos)
{
	_posInfo->CopyFrom(respawnPos);
    // 서버가 위치를 정했으므로 이동 판정의 기준 시각도 지금으로 둔다.
    _lastPositionTick = ::GetTickCount64();

    RespawnResult result;

	switch (type)
	{
	case Protocol::RESPAWN_TYPE_TOWN:
	{
		// 사망 패널티. 그 레벨 요구치의 10%를 잃고, 레벨은 내려가지 않는다.
		if (const ClassLevelTable* classLevelTable = Gamedata::FindClassLevelTable(_characterClass))
		{
			SetTotalExp(*classLevelTable, Leveling::ApplyDeathPenalty(*classLevelTable, _totalExp));

			ProtoUtil::AddStat(&result.updatedStats, Protocol::STAT_TYPE_EXP, GetStatValue(Protocol::STAT_TYPE_EXP));
		}

		// HP는 최대치의 절반으로 되살아난다.
		{
			int64 respawnHp = (int64)(GetStatValue(Protocol::STAT_TYPE_MAX_HP) * 0.5f);

			SetStatValue(Protocol::STAT_TYPE_HP, respawnHp);
			ProtoUtil::AddStat(&result.updatedStats, Protocol::STAT_TYPE_HP, respawnHp);
		}

		break;
	}
	default:
		break;
	}

	{
		_isDead = false;
	}

	return result;
}

void Player::OnHit(EntityRef attacker, const AttackInfo& attackInfo)
{
    Creature::OnHit(attacker, attackInfo);

}

void Player::OnDie(EntityRef attacker)
{
    Creature::OnDie(attacker);

}

void Player::BeginMapLoad(const RoomRef& room, const Protocol::PosInfo& pos)
{
    // 기다리는 동안의 접속 종료는 이 맵과 위치로 저장한다. 목적지를 정한 뒤에 끊겨도 다음 입장이 그 맵에서 시작한다.
    _mapId = room->GetMapId();
    _posInfo->CopyFrom(pos);
    _mapLoadRoomId = room->GetRoomId();

    // 소속 룸을 목적지로 바꿔 두면 맵 로드 완료와 접속 종료가 목적지 룸의 큐 하나에서 차례로 돈다.
    // 기다리는 동안 온 다른 요청도 그 큐로 가서, 룸에 없는 플레이어의 요청으로 버려진다.
    AssignRoom(room);
}

bool Player::FinishMapLoad(int32 roomId)
{
    // 룸 번호는 -1이 아니므로 기다리지 않는 플레이어는 여기서 걸린다.
    if (_mapLoadRoomId != roomId)
        return false;

    _mapLoadRoomId = -1;
    return true;
}

void Player::OnEnterRoom(RoomRef enterRoom, const optional<Protocol::PosInfo>& enterPos)
{
    _roomId = enterRoom->GetRoomId();

    if(enterPos.has_value())
    {
        _posInfo->CopyFrom(enterPos.value());
    }
    else
    {
        const vector3D& centerPos = enterRoom->GetCenterPoint();
        Protocol::Vector* pos = _posInfo->mutable_pos();

        pos->set_x(centerPos.x);
        pos->set_y(centerPos.y);
        pos->set_z(centerPos.z);
        _posInfo->set_yaw(0.f);
        _posInfo->set_state(Protocol::MOVE_STATE_IDLE);
    }

    _lastPositionTick = ::GetTickCount64();
}

void Player::ApplyMove(const Protocol::PosInfo& posInfo, uint64 now)
{
    _posInfo->CopyFrom(posInfo);
    _lastPositionTick = now;
}

RewardResult Player::OnGetReward(const Reward& reward)
{
    const int32 oldLevel = _playerInfo->level();

    {
        _gold += reward.gold;

        // 경험치는 누적값에 더하고, 레벨은 누적 경험치로 다시 정한다. 보상 한 번에 여러 레벨이 오를 수 있다.
        // 최대 레벨에 이르고 남는 경험치는 버린다. 레벨 표가 없는 직업은 오를 레벨이 없으므로 경험치를 쌓지 않는다.
        if (const ClassLevelTable* classLevelTable = Gamedata::FindClassLevelTable(_characterClass))
        {
            const LevelExp levelExp = SetTotalExp(*classLevelTable, Leveling::AddExp(*classLevelTable, _totalExp, reward.exp));
            while (_playerInfo->level() < levelExp.level)
                OnLevelUp();
        }
    }

    RewardResult result;
    {
        result.exp = GetStatValue(Protocol::STAT_TYPE_EXP);
        result.gold = _gold;

        if (_playerInfo->level() > oldLevel)
        {
            LevelUpResult& levelUp = result.levelUp.emplace();
            levelUp.level = _playerInfo->level();

			RepeatedPtrField<Protocol::Stat>* updatedStatList = &levelUp.changedStats;
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

    return result;
}

void Player::OnLevelUp()
{
    if (IsMaxLevel())
        return;

    _playerInfo->set_level(_playerInfo->level() + 1);

    SetStatValue(Protocol::STAT_TYPE_MAX_HP, GetStatValue(Protocol::STAT_TYPE_MAX_HP) + _nextLevelUpData.maxHpIncrement);
    SetStatValue(Protocol::STAT_TYPE_MAX_MP, GetStatValue(Protocol::STAT_TYPE_MAX_MP) + _nextLevelUpData.maxMpIncrement);
    SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK) + _nextLevelUpData.paIncrement);
    SetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK, GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK) + _nextLevelUpData.maIncrement);

    CacheNextLevelUpData();
}

bool Player::IsMaxLevel() const
{
    // 레벨 표가 없는 직업은 오를 레벨이 없으므로 최대 레벨로 본다.
    const ClassLevelTable* classLevelTable = Gamedata::FindClassLevelTable(_characterClass);
    return classLevelTable == nullptr || _playerInfo->level() >= classLevelTable->GetMaxLevel();
}

void Player::CacheNextLevelUpData()
{
    // 최대 레벨에서는 오를 레벨이 없으므로 캐시를 그대로 둔다. 레벨 표가 없는 직업도 여기서 끝난다.
    if (IsMaxLevel())
        return;

    // 다음 레벨 행이 없으면 빈 값으로 둔다.
    _nextLevelUpData = NextLevelUpData{};

    const ClassLevelTable* classLevelTable = Gamedata::FindClassLevelTable(_characterClass);
    const LevelTemplate* nextLevelTemplate = classLevelTable != nullptr ? classLevelTable->Find(_playerInfo->level() + 1) : nullptr;
    if (nextLevelTemplate == nullptr)
        return;

    _nextLevelUpData.maxHpIncrement = nextLevelTemplate->maxHpIncrement;
    _nextLevelUpData.maxMpIncrement = nextLevelTemplate->maxMpIncrement;
    _nextLevelUpData.paIncrement = nextLevelTemplate->paIncrement;
    _nextLevelUpData.maIncrement = nextLevelTemplate->maIncrement;
}

LevelExp Player::SetTotalExp(const ClassLevelTable& classLevelTable, int64 totalExp)
{
    _totalExp = totalExp;

    const LevelExp levelExp = Leveling::Locate(classLevelTable, _totalExp);
    SetStatValue(Protocol::STAT_TYPE_EXP, levelExp.expInLevel);
    SetStatValue(Protocol::STAT_TYPE_MAX_EXP, levelExp.expRequirement);

    return levelExp;
}

optional<CombatStats> Player::CalculateFinalStat()
{
    const ClassLevelTable* classLevelTable = Gamedata::FindClassLevelTable(_characterClass);
    if (classLevelTable == nullptr)
    {
        GLogger->Error("스텟 계산에 문제가 생겼습니다. 사유: 레벨 표에 없는 직업 {}", static_cast<int32>(_characterClass));
        return nullopt;
    }

    const LevelTemplate* levelTemplate = classLevelTable->Find(_playerInfo->level());
    if (levelTemplate == nullptr)
    {
        GLogger->Error("스텟 계산에 문제가 생겼습니다. 사유: 레벨 표에 없는 레벨 {}", _playerInfo->level());
        return nullopt;
    }

    CombatStats finalStat{ levelTemplate->maxHp, levelTemplate->maxMp, levelTemplate->physicalAttack, levelTemplate->magicalAttack };

    // 스킬 패시브, 내실 등 캐릭터 스탯을 올릴 수 있는 요소가 추가되면 장비처럼 여기에 더한다.
    finalStat += _equipment->SumStatDelta();

    return finalStat;
}

void Player::RefreshFinalStat(OUT RepeatedPtrField<Protocol::Stat>* updatedStats)
{
    const optional<CombatStats> finalStat = CalculateFinalStat();
    if (finalStat.has_value() == false)
        return;

    // 값이 바뀐 스탯만 싣는다. 클라이언트는 받은 스탯만 고친다.
    auto apply = [&](Protocol::StatType statType, int64 value)
        {
            const bool changed = HasStat(statType) == false || GetStatValue(statType) != value;
            SetStatValue(statType, value);

            if (changed && updatedStats != nullptr)
                ProtoUtil::AddStat(updatedStats, statType, value);
        };

    apply(Protocol::STAT_TYPE_MAX_HP, finalStat->maxHp);
    apply(Protocol::STAT_TYPE_MAX_MP, finalStat->maxMp);
    apply(Protocol::STAT_TYPE_PHYSICAL_ATTACK, finalStat->physicalAttack);
    apply(Protocol::STAT_TYPE_MAGICAL_ATTACK, finalStat->magicalAttack);

    // 최대치가 줄면 현재 값도 따라 줄인다. 넘는 채로 저장되면 다음 입장의 대조가 거절한다.
    if (HasStat(Protocol::STAT_TYPE_HP) && GetStatValue(Protocol::STAT_TYPE_HP) > finalStat->maxHp)
        apply(Protocol::STAT_TYPE_HP, finalStat->maxHp);
    if (HasStat(Protocol::STAT_TYPE_MP) && GetStatValue(Protocol::STAT_TYPE_MP) > finalStat->maxMp)
        apply(Protocol::STAT_TYPE_MP, finalStat->maxMp);
}

void Player::RefreshEquippedGearSummary()
{
    // 다른 플레이어는 장비 슬롯을 받지 못하므로, 외형에 필요한 부위와 템플릿만 공개 정보에 싣는다.
    auto* summary = _playerInfo->mutable_gear_appearance();
    summary->clear();

    const ItemContainer& equipped = _equipment->GetContainer();
    for (int32 gearType : equipped.GetSlotIndices())
    {
        if (const ItemInstance* item = equipped.Find(gearType))
            (*summary)[gearType] = item->templateId;
    }
}

vector<SlotChange> Player::TakeItemChanges()
{
    vector<SlotChange> changes;
    _equipment->TakeChanges(changes);
    _inventory->TakeChanges(changes);
    return changes;
}

bool Player::MarkDisconnected()
{
    // 룸 큐의 MarkEnteredRoom과 같은 값을 바꾼다. 읽고 바꾸는 일을 한 번에 해서 둘 중 한쪽 판정만 남긴다.
    Presence current = _presence.load();
    while (true)
    {
        Presence next = current;
        if (current == Presence::LOADED)
            next = Presence::LEFT_BEFORE_ROOM;
        else if (current == Presence::IN_ROOM)
            next = Presence::LEFT_IN_ROOM;

        if (next == current || _presence.compare_exchange_weak(current, next))
            return next == Presence::LEFT_IN_ROOM;
    }
}

Player::Presence Player::MarkEnteredRoom()
{
    // 실패하면 expected에 지금 값이 들어온다. 성공하면 LOADED가 남으므로 바꾼 값을 따로 돌려준다.
    Presence expected = Presence::LOADED;
    if (_presence.compare_exchange_strong(expected, Presence::IN_ROOM))
        return Presence::IN_ROOM;

    return expected;
}

PlayerSaveData Player::MakeSaveData() const
{
    PlayerSaveData data;
    data.userId = _userId;
    data.progress.characterId = _characterId;
    data.progress.classId = _characterClass;
    data.progress.name = _playerInfo->name();
    data.progress.level = _playerInfo->level();
    data.progress.mapId = _mapId;
    data.progress.pos = vector3D(_posInfo->pos().x(), _posInfo->pos().y(), _posInfo->pos().z());
    data.progress.yaw = _posInfo->yaw();
    // DB에는 누적 경험치를 저장한다. 스탯의 경험치는 클라이언트에 보낼 레벨 안의 값이다.
    data.progress.totalExp = _totalExp;
    // DB 열이 INT다. 최대치가 레벨 표의 int32 값에서 오므로 넘지 않는다.
    const auto& stats = _statInfo->values();
    const auto hpIt = stats.find(Protocol::STAT_TYPE_HP);
    const auto mpIt = stats.find(Protocol::STAT_TYPE_MP);
    data.progress.hp = hpIt != stats.end() ? static_cast<int32>(hpIt->second) : 0;
    data.progress.mp = mpIt != stats.end() ? static_cast<int32>(mpIt->second) : 0;
    data.progress.gold = _gold;
    _inventory->CollectItems(data.progress.items);
    _equipment->CollectItems(data.progress.items);
    _quickSlots->CollectQuickSlots(data.progress.quickSlots);

    return data;
}

// 사망한 채 접속이 끊기면 사망 화면에서 마을 리스폰을 누른 것과 같은 상태로 저장한다.
// 사망 여부는 저장되지 않으므로, 그대로 저장하면 다시 접속했을 때 HP 0으로 살아서 들어온다.
// 저장은 룸에서 뺀 뒤에 뜬다. 그래서 소속 룸이나 전역 룸 관리자를 읽지 않고 호출자가 맵 표에서 찾은 마을을 받는다.
void Player::ApplyTownRespawnForSave(const TownRespawn& town)
{
    ProcessRespawn(Protocol::RESPAWN_TYPE_TOWN, town.pos);

    // DB에는 룸이 아니라 맵과 좌표를 저장한다. 맵을 바꾸지 않으면 다음 입장이 죽은 맵의 마을 좌표에서 룸을 찾는다.
    _mapId = town.mapId;
    _roomId = town.roomId;
}
