#include "Game/Progress/P1MyPlayerData.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/P1ItemData.h"
#include "Game/Inventory/P1Inventory.h"
#include "Game/Equipment/P1Equipment.h"
#include "Utils/LogCategory.h"

UP1MyPlayerData::UP1MyPlayerData()
{
    // 프로토콜 사본과 스탯 델리게이트 자리는 서브시스템이 초기화되기 전에도 쓸 수 있게 여기서 만든다.
    _EntityInfo = MakeUnique<Protocol::EntityInfo>();
    _PlayerInfo = _EntityInfo->mutable_player_info();
    _StatInfo = MakeUnique<Protocol::StatInfo>();

    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MAX_HP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_HP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MAX_MP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_PHYSICAL_ATTACK);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MAGICAL_ATTACK);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MAX_EXP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_EXP);
}

void UP1MyPlayerData::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 인벤토리 생성.
    Inventory = NewObject<UP1Inventory>(this, UP1Inventory::StaticClass());
    if (Inventory == nullptr)
        UE_LOG(LogP1CharacterComp, Warning, TEXT("Inventory Is Not Exist"));

    // 장비 생성.
    Equipment = NewObject<UP1Equipment>(this, UP1Equipment::StaticClass());
    if (Equipment == nullptr)
        UE_LOG(LogP1CharacterComp, Warning, TEXT("Equipment Is Not Exist"));

    // 소지품 델리게이트는 이 서브시스템과 함께 사는 객체끼리 잇는다. 플레이어 액터와 무관하므로 한 번만 붙인다.
    // 내 플레이어가 스폰될 때마다 붙이면 맵을 옮길 때마다 핸들러가 하나씩 늘어난다.
    OnInvenSlotChanged.AddUObject(Inventory, &UP1Inventory::Rep_SlotChanged);
    OnEquipmentSlotChanged.AddUObject(Equipment, &UP1Equipment::Rep_SlotChanged);
}

void UP1MyPlayerData::Deinitialize()
{
    Inventory = nullptr;
    Equipment = nullptr;

    // _PlayerInfo는 _EntityInfo 안을 가리키므로 먼저 끊는다.
    _PlayerInfo = nullptr;
    _EntityInfo.Reset();
    _StatInfo.Reset();

    Super::Deinitialize();
}

void UP1MyPlayerData::InitMyPlayerData(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
    // 입장 패킷의 캐릭터 정보 저장.
    {
        SetEntityInfo(EnterGamePkt.player());
        _StatInfo->CopyFrom(EnterGamePkt.stat_info());
        _Gold = EnterGamePkt.possession().gold();

        _PlayerId = _EntityInfo->entity_id();
        _PlayerName = FText::FromString(UTF8_TO_TCHAR(_EntityInfo->player_info().name().c_str()));
    }

    // 소지품 래퍼 초기화.
    {
        Inventory->Init(EnterGamePkt.possession());
        Equipment->Init(EnterGamePkt.possession());
    }

    // 서버는 재접속하면 재사용 대기를 잊는다. 같은 시점에 비워 둘의 판정을 맞춘다.
    ItemCooldowns.Reset();

}

void UP1MyPlayerData::RemoveListener(const UObject* Listener)
{
    OnMyPlayerSpawned.RemoveAll(Listener);
    OnMapEntered.RemoveAll(Listener);
    OnLevelChanged.RemoveAll(Listener);
    OnGoldChanged.RemoveAll(Listener);
    OnInvenSlotChanged.RemoveAll(Listener);
    OnEquipmentSlotChanged.RemoveAll(Listener);
    OnRecvBuyItemPkt.RemoveAll(Listener);
    OnRecvSellItemPkt.RemoveAll(Listener);
    OnRecvUseItemPkt.RemoveAll(Listener);
    OnRecvEquipGearPkt.RemoveAll(Listener);
    OnRecvUnequipGearPkt.RemoveAll(Listener);
    OnItemCooldownStarted.RemoveAll(Listener);

    for (auto& Pair : OnStatChangedMappings)
        Pair.Value.RemoveAll(Listener);
}

void UP1MyPlayerData::SetEntityInfo(const Protocol::EntityInfo& InEntityInfo)
{
    // 프로토버프는 CopyFrom 뒤에 하위 메시지의 주소가 달라질 수 있다. _PlayerInfo를 다시 잡지 않으면 옛 주소를 읽는다.
    _EntityInfo->CopyFrom(InEntityInfo);
    _PlayerInfo = _EntityInfo->mutable_player_info();
}

void UP1MyPlayerData::SetStatValue(Protocol::StatType statType, const int64& value)
{
    auto* statMappings = _StatInfo->mutable_info();
    (*statMappings)[(int32)statType] = value;
}

void UP1MyPlayerData::ApplyStat(Protocol::StatType statType, int64 value)
{
    // 알리기만 하고 사본에 쓰지 않으면 안 된다. HUD는 생성될 때 사본을 읽으므로 맵을 옮긴 뒤 옛 값이 보인다.
    SetStatValue(statType, value);

    if (FOnStatChanged* OnThisStatChanged = OnStatChangedMappings.Find(statType))
        OnThisStatChanged->Broadcast(value);
}

void UP1MyPlayerData::ApplyStats(const google::protobuf::RepeatedPtrField<Protocol::Stat>& Stats)
{
    for (const Protocol::Stat& Stat_ : Stats)
        ApplyStat(Stat_.type(), Stat_.value());
}

int64 UP1MyPlayerData::GetStatValue(Protocol::StatType statType)
{
    auto* statMappings = _StatInfo->mutable_info();
    return statMappings->at((int32)statType);
}

void UP1MyPlayerData::ApplyLevel(int32 Level)
{
    _PlayerInfo->set_level(Level);
    OnLevelChanged.Broadcast(Level);
}

void UP1MyPlayerData::ApplyGold(int64 Gold)
{
    _Gold = Gold;
    OnGoldChanged.Broadcast(Gold);
}

void UP1MyPlayerData::HandleEnterMap(const Protocol::S_ENTER_MAP& EnterMapPkt)
{
    if (EnterMapPkt.success() == false)
    {
        UE_LOG(LogP1Network, Warning, TEXT("맵 입장에 실패했습니다. map_id: %d"), EnterMapPkt.map_id());
        return;
    }

    SetRoomId(EnterMapPkt.room_id());
    SetMapId(EnterMapPkt.map_id());

    // 게임 인스턴스가 이 알림을 받아 인게임 맵을 연다.
    OnMapEntered.Broadcast();
}

void UP1MyPlayerData::HandleEnterRoom(const Protocol::S_ENTER_ROOM& EnterRoomPkt)
{
    if (EnterRoomPkt.success() == false)
    {
        UE_LOG(LogP1Network, Warning, TEXT("Room 입장에 실패했습니다. room_id: %d"), EnterRoomPkt.room_id());
        return;
    }

    SetRoomId(EnterRoomPkt.room_id());
}

void UP1MyPlayerData::HandleBuyItem(const Protocol::S_BUY_ITEM& BuyItemPkt)
{
    OnRecvBuyItemPkt.Broadcast();
    if (BuyItemPkt.success() == false)
        return;

    ApplySlotChanges(BuyItemPkt.changed_slots());
    ApplyGold(BuyItemPkt.gold());
}

void UP1MyPlayerData::HandleSellItem(const Protocol::S_SELL_ITEM& SellItemPkt)
{
    OnRecvSellItemPkt.Broadcast();
    if (SellItemPkt.success() == false)
        return;

    ApplySlotChanges(SellItemPkt.changed_slots());
    ApplyGold(SellItemPkt.gold());
}

void UP1MyPlayerData::HandleUseItem(const Protocol::S_USE_ITEM& UseItemPkt)
{
    if (IsMyPlayer(UseItemPkt.entity_id()) == false)
        return;

    OnRecvUseItemPkt.Broadcast();
    if (UseItemPkt.success() == false)
        return;

    // 마지막 한 개를 쓰면 응답의 칸에 아이템이 없다. 사본이 바뀌기 전에 쓴 아이템의 템플릿을 읽는다.
    TArray<int32, TInlineAllocator<1>> UsedTemplateIds;
    for (const Protocol::SlotChange& Change : UseItemPkt.changed_slots())
    {
        const Protocol::SlotChange* Before = Inventory->FindSlot(Change.address().container(), Change.address().slot_index());
        if (Before && Before->has_item())
            UsedTemplateIds.Add(Before->item().template_id());
    }

    ApplySlotChanges(UseItemPkt.changed_slots());
    ApplyStats(UseItemPkt.updated_stat());

    // 칸이 먼저 바뀌어야 대기를 알릴 때 칸들이 지금 든 아이템으로 판단한다.
    for (const int32 TemplateId : UsedTemplateIds)
        StartItemCooldown(TemplateId);
}

const FP1ItemCooldown* UP1MyPlayerData::FindActiveItemCooldown(int32 TemplateId) const
{
    const FP1ItemCooldown* Cooldown = ItemCooldowns.Find(TemplateId);
    if (Cooldown && Cooldown->IsCoolingDown(FP1ItemCooldown::GetClockSeconds()))
        return Cooldown;

    return nullptr;
}

void UP1MyPlayerData::StartItemCooldown(int32 TemplateId)
{
    const FP1ItemData* ItemData = UP1GameDataSettings::FindItemData(TemplateId);
    if (ItemData == nullptr || ItemData->Cooldown <= 0.f)
        return;

    // 응답을 받은 지금부터 센다. 서버는 요청을 처리한 시각부터 세므로 클라이언트의 대기가 늘 조금 늦게 끝나고,
    // 클라이언트가 쓸 수 있다고 보일 때 서버가 거절하는 일이 없다.
    ItemCooldowns.Add(TemplateId, FP1ItemCooldown{ FP1ItemCooldown::GetClockSeconds(), ItemData->Cooldown });
    OnItemCooldownStarted.Broadcast(TemplateId);
}

void UP1MyPlayerData::HandleEquipGear(const Protocol::S_EQUIP_GEAR& EquipGearPkt)
{
    if (IsMyPlayer(EquipGearPkt.entity_id()) == false)
        return;

    OnRecvEquipGearPkt.Broadcast();
    if (EquipGearPkt.success() == false)
        return;

    ApplySlotChanges(EquipGearPkt.changed_slots());
    ApplyStats(EquipGearPkt.updated_stat());
}

void UP1MyPlayerData::HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt)
{
    if (IsMyPlayer(UnequipGearPkt.entity_id()) == false)
        return;

    OnRecvUnequipGearPkt.Broadcast();
    if (UnequipGearPkt.success() == false)
        return;

    ApplySlotChanges(UnequipGearPkt.changed_slots());
    ApplyStats(UnequipGearPkt.updated_stat());
}

void UP1MyPlayerData::ApplySlotChanges(const google::protobuf::RepeatedPtrField<Protocol::SlotChange>& Changes)
{
    for (const Protocol::SlotChange& Change : Changes)
    {
        switch (Change.address().container())
        {
        case Protocol::CONTAINER_TYPE_EQUIPPED:
            OnEquipmentSlotChanged.Broadcast(Change);
            break;

        case Protocol::CONTAINER_TYPE_INVENTORY_GEAR:
        case Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE:
        case Protocol::CONTAINER_TYPE_INVENTORY_MISC:
            OnInvenSlotChanged.Broadcast(Change);
            break;

        default:
            break;
        }
    }
}

void UP1MyPlayerData::HandleRewardResult(const Protocol::S_REWARD_RESULT& RewardResultPkt)
{
    // 레벨과 레벨업 스탯을 경험치보다 먼저 반영한다. 경험치를 알릴 때 최대 경험치가 이미 새 값이어야
    // 구독자가 새 경험치를 옛 최대 경험치에 대어 그리지 않는다.
    if (RewardResultPkt.is_level_up())
    {
        const Protocol::LevelUpInfo& LevelUpInfo = RewardResultPkt.level_up_details();
        ApplyLevel(LevelUpInfo.new_level());
        ApplyStats(LevelUpInfo.updated_stat());
    }

    ApplyStat(Protocol::STAT_TYPE_EXP, RewardResultPkt.updated_exp());
    ApplyGold(RewardResultPkt.updated_gold());
}
