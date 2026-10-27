#include "Game/Progress/P1MyPlayerData.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/Generated/P1ItemConsumeEffectRow.h"
#include "Game/Items/P1ItemContainer.h"
#include "Game/Items/P1ItemPackets.h"
#include "Game/Progress/P1ItemActions.h"
#include "Game/QuickSlots/P1QuickSlotPackets.h"
#include "Game/QuickSlots/P1QuickSlots.h"
#include "Utils/LogCategory.h"

namespace
{
    /** 내 플레이어가 가진 컨테이너. 입장 패킷의 소지품과 응답의 칸 변경이 이 종류로 나뉜다. */
    constexpr EP1ContainerType CONTAINER_TYPES[] = {
        EP1ContainerType::InventoryGear,
        EP1ContainerType::InventoryConsumable,
        EP1ContainerType::InventoryMisc,
        EP1ContainerType::Equipped,
    };

    /** 서버가 받아들인 응답이면 true. 거절이면 결과를 로그에 남긴다. 화면 문구는 서버가 보내지 않는다. */
    bool IsAccepted(Protocol::ItemResult Result, const TCHAR* RequestName)
    {
        if (Result == Protocol::ITEM_RESULT_SUCCESS)
            return true;

        UE_LOG(LogP1CharacterInventory, Warning, TEXT("%s 요청이 거절됐습니다: %s"), RequestName, UTF8_TO_TCHAR(Protocol::ItemResult_Name(Result).c_str()));
        return false;
    }
}

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

    // 컨테이너도 서브시스템이 초기화되기 전에 쓸 수 있게 여기서 만든다.
    for (const EP1ContainerType ContainerType : CONTAINER_TYPES)
    {
        const FString TypeName = StaticEnum<EP1ContainerType>()->GetNameStringByValue(static_cast<int64>(ContainerType));
        UP1ItemContainer* Container = CreateDefaultSubobject<UP1ItemContainer>(*FString::Printf(TEXT("Container_%s"), *TypeName));
        Container->Init(ContainerType);
        Containers.Add(ContainerType, Container);
    }

    QuickSlots = CreateDefaultSubobject<UP1QuickSlots>(TEXT("QuickSlots"));
    ItemActions = CreateDefaultSubobject<UP1ItemActions>(TEXT("ItemActions"));
}

void UP1MyPlayerData::Deinitialize()
{
    // _PlayerInfo는 _EntityInfo 안을 가리키므로 먼저 끊는다.
    _PlayerInfo = nullptr;
    _EntityInfo.Reset();
    _StatInfo.Reset();

    Super::Deinitialize();
}

void UP1MyPlayerData::InitMyPlayerData(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
    // 입장 패킷의 캐릭터 정보 저장. 맵을 여는 동안 만들어지는 화면이 이름과 레벨을 읽는다.
    // 내 엔티티 번호는 룸 입장 응답이 알린다. 지우지 않으면 앞 캐릭터의 번호로 새 캐릭터의 스폰을 가린다.
    {
        _EntityInfo->Clear();
        _PlayerInfo = _EntityInfo->mutable_player_info();
        _PlayerInfo->set_name(EnterGamePkt.name());
        _PlayerInfo->set_level(EnterGamePkt.level());
        _StatInfo->CopyFrom(EnterGamePkt.stats());
        _Gold = EnterGamePkt.possession().gold();
        _MapId = EnterGamePkt.map_id();

        _PlayerId = 0;
        _PlayerName = FText::FromString(UTF8_TO_TCHAR(EnterGamePkt.name().c_str()));
    }

    // 컨테이너 채우기. 패킷에 없는 컨테이너도 비워 이전 캐릭터의 칸이 남지 않게 한다.
    for (const auto& Pair : Containers)
    {
        TArray<FP1Slot> Slots;
        for (const Protocol::ContainerSlots& ContainerSlots : EnterGamePkt.possession().containers())
        {
            if (P1ItemPackets::FromProto(ContainerSlots.container()) == Pair.Key)
            {
                Slots = P1ItemPackets::FromProto(ContainerSlots);
                break;
            }
        }
        Pair.Value->ResetSlots(Slots);
    }

    // 퀵슬롯 채우기. 퀵슬롯 개수는 서버가 기획 데이터에서 읽어 빈 퀵슬롯까지 싣는다.
    {
        TArray<FP1QuickSlot> Received;
        for (const Protocol::QuickSlot& QuickSlot : EnterGamePkt.quick_slots())
            Received.Add(P1QuickSlotPackets::FromProto(QuickSlot));
        QuickSlots->ResetQuickSlots(Received);
    }

    // 서버는 재접속하면 재사용 대기를 잊는다. 같은 시점에 비워 둘의 판정을 맞춘다.
    ItemCooldowns.Reset();

    // 끊기기 전에 보낸 요청의 응답은 오지 않는다. 풀지 않으면 그 종류의 요청이 계속 막힌다.
    ItemActions->ResetPending();
}

void UP1MyPlayerData::RemoveListener(const UObject* Listener)
{
    OnMyPlayerSpawned.RemoveAll(Listener);
    OnMapChanged.RemoveAll(Listener);
    OnLevelChanged.RemoveAll(Listener);
    OnGoldChanged.RemoveAll(Listener);
    OnItemCooldownStarted.RemoveAll(Listener);

    for (auto& Pair : OnStatChangedMappings)
        Pair.Value.RemoveAll(Listener);

    for (const auto& Pair : Containers)
    {
        Pair.Value->OnSlotChanged.RemoveAll(Listener);
        Pair.Value->OnReset.RemoveAll(Listener);
    }

    QuickSlots->OnChanged.RemoveAll(Listener);
    QuickSlots->OnReset.RemoveAll(Listener);
}

void UP1MyPlayerData::SetEntityInfo(const Protocol::EntityInfo& InEntityInfo)
{
    // 프로토버프는 CopyFrom 뒤에 하위 메시지의 주소가 달라질 수 있다. _PlayerInfo를 다시 잡지 않으면 옛 주소를 읽는다.
    _EntityInfo->CopyFrom(InEntityInfo);
    _PlayerInfo = _EntityInfo->mutable_player_info();
}

void UP1MyPlayerData::SetStatValue(Protocol::StatType statType, const int64& value)
{
    auto* statMappings = _StatInfo->mutable_values();
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
    auto* statMappings = _StatInfo->mutable_values();
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

UP1ItemContainer* UP1MyPlayerData::GetContainer(EP1ContainerType ContainerType) const
{
    const TObjectPtr<UP1ItemContainer>* Found = Containers.Find(ContainerType);
    return Found ? Found->Get() : nullptr;
}

void UP1MyPlayerData::ClearContainers()
{
    for (const auto& Pair : Containers)
        Pair.Value->Clear();
}

void UP1MyPlayerData::ClearQuickSlots()
{
    QuickSlots->Clear();
}

void UP1MyPlayerData::HandleSetQuickSlot(const Protocol::S_SET_QUICK_SLOT& SetQuickSlotPkt)
{
    ItemActions->HandleResponse(UP1ItemActions::ERequestKind::SetQuickSlot);
    if (IsAccepted(SetQuickSlotPkt.result(), TEXT("퀵슬롯 등록")) == false)
        return;

    for (const Protocol::QuickSlot& Changed : SetQuickSlotPkt.changed_quick_slots())
        QuickSlots->ApplyChange(P1QuickSlotPackets::FromProto(Changed));
}

void UP1MyPlayerData::HandleEnterRoom(const Protocol::S_ENTER_ROOM& EnterRoomPkt)
{
    if (EnterRoomPkt.result() != Protocol::ENTRY_RESULT_SUCCESS)
    {
        UE_LOG(LogP1Network, Warning, TEXT("룸 입장에 실패했습니다: %s"), UTF8_TO_TCHAR(Protocol::EntryResult_Name(EnterRoomPkt.result()).c_str()));
        return;
    }

    // 엔티티 관리자와 스포너가 이 번호로 내 플레이어를 가린다. 엔티티 관리자보다 먼저 불려야 한다.
    _PlayerId = EnterRoomPkt.my_entity().entity_id();
}

void UP1MyPlayerData::HandleUsePortal(const Protocol::S_USE_PORTAL& UsePortalPkt)
{
    if (UsePortalPkt.result() != Protocol::ENTRY_RESULT_SUCCESS)
    {
        UE_LOG(LogP1Network, Warning, TEXT("포털 이동이 거절됐습니다: %s"), UTF8_TO_TCHAR(Protocol::EntryResult_Name(UsePortalPkt.result()).c_str()));
        return;
    }

    // 맵 번호가 없으면 같은 맵 안의 이동이라 서버가 곧바로 룸 입장 응답을 보낸다.
    if (UsePortalPkt.has_map_id())
        ChangeMap(UsePortalPkt.map_id());
}

void UP1MyPlayerData::HandleRespawn(const Protocol::S_RESPAWN& RespawnPkt)
{
    if (RespawnPkt.result() != Protocol::ENTRY_RESULT_SUCCESS)
    {
        UE_LOG(LogP1Network, Error, TEXT("서버가 리스폰을 거절했습니다: %s"), UTF8_TO_TCHAR(Protocol::EntryResult_Name(RespawnPkt.result()).c_str()));
        OnTownRespawnRejected.Broadcast();
        return;
    }

    ApplyStats(RespawnPkt.changed_stats());

    if (RespawnPkt.has_map_id())
        ChangeMap(RespawnPkt.map_id());
}

void UP1MyPlayerData::ChangeMap(int32 MapId)
{
    _MapId = MapId;

    // 접속 흐름이 이 알림을 받아 그 맵을 연다.
    OnMapChanged.Broadcast();
}

void UP1MyPlayerData::HandleBuyItem(const Protocol::S_BUY_ITEM& BuyItemPkt)
{
    ItemActions->HandleResponse(UP1ItemActions::ERequestKind::Buy);
    if (IsAccepted(BuyItemPkt.result(), TEXT("구매")) == false)
        return;

    ApplySlotChanges(BuyItemPkt.changed_slots());
    ApplyGold(BuyItemPkt.gold());
}

void UP1MyPlayerData::HandleSellItem(const Protocol::S_SELL_ITEM& SellItemPkt)
{
    ItemActions->HandleResponse(UP1ItemActions::ERequestKind::Sell);
    if (IsAccepted(SellItemPkt.result(), TEXT("판매")) == false)
        return;

    ApplySlotChanges(SellItemPkt.changed_slots());
    ApplyGold(SellItemPkt.gold());
}

void UP1MyPlayerData::HandleUseItem(const Protocol::S_USE_ITEM& UseItemPkt)
{
    ItemActions->HandleResponse(UP1ItemActions::ERequestKind::Use);
    if (IsAccepted(UseItemPkt.result(), TEXT("소모품 사용")) == false)
        return;

    // 마지막 한 개를 쓰면 응답의 칸에 아이템이 없다. 사본이 바뀌기 전에 쓴 아이템의 템플릿을 읽는다.
    TArray<int32, TInlineAllocator<1>> UsedTemplateIds;
    for (const Protocol::SlotChange& Change : UseItemPkt.changed_slots())
    {
        const FP1SlotAddress Address = P1ItemPackets::FromProto(Change.address());
        const UP1ItemContainer* Container = GetContainer(Address.Container);
        const FP1Slot* Before = Container ? Container->FindSlot(Address.SlotIndex) : nullptr;
        if (Before && Before->HasItem())
            UsedTemplateIds.Add(Before->Item.TemplateId);
    }

    ApplySlotChanges(UseItemPkt.changed_slots());
    ApplyStats(UseItemPkt.changed_stats());

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
    // 재사용 대기는 소모품에만 있다.
    const FP1ItemConsumeEffectRow* Effect = UP1GameDataSettings::FindItemConsumeEffectRow(TemplateId);
    if (Effect == nullptr || Effect->CooldownMs <= 0)
        return;

    // 응답을 받은 지금부터 센다. 서버는 요청을 처리한 시각부터 세므로 클라이언트의 대기가 늘 조금 늦게 끝나고,
    // 클라이언트가 쓸 수 있다고 보일 때 서버가 거절하는 일이 없다.
    ItemCooldowns.Add(TemplateId, FP1ItemCooldown{ FP1ItemCooldown::GetClockSeconds(), Effect->CooldownMs / 1000.0 });
    OnItemCooldownStarted.Broadcast(TemplateId);
}

void UP1MyPlayerData::HandleEquipGear(const Protocol::S_EQUIP_GEAR& EquipGearPkt)
{
    ItemActions->HandleResponse(UP1ItemActions::ERequestKind::Equip);
    if (IsAccepted(EquipGearPkt.result(), TEXT("착용")) == false)
        return;

    ApplySlotChanges(EquipGearPkt.changed_slots());
    ApplyStats(EquipGearPkt.changed_stats());
}

void UP1MyPlayerData::HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt)
{
    ItemActions->HandleResponse(UP1ItemActions::ERequestKind::Unequip);
    if (IsAccepted(UnequipGearPkt.result(), TEXT("해제")) == false)
        return;

    ApplySlotChanges(UnequipGearPkt.changed_slots());
    ApplyStats(UnequipGearPkt.changed_stats());
}

void UP1MyPlayerData::ApplySlotChanges(const google::protobuf::RepeatedPtrField<Protocol::SlotChange>& Changes)
{
    for (const Protocol::SlotChange& Change : Changes)
    {
        const FP1Slot Changed = P1ItemPackets::FromProto(Change);
        if (UP1ItemContainer* Container = GetContainer(Changed.Address.Container))
            Container->ApplySlotChange(Changed);
        else
            UE_LOG(LogP1CharacterInventory, Warning, TEXT("컨테이너가 아닌 칸의 변경을 버린다. 컨테이너 %d"), static_cast<int32>(Change.address().container()));
    }
}

void UP1MyPlayerData::HandleReward(const Protocol::S_REWARD& RewardPkt)
{
    // 레벨과 레벨업 스탯을 경험치보다 먼저 반영한다. 경험치를 알릴 때 최대 경험치가 이미 새 값이어야
    // 구독자가 새 경험치를 옛 최대 경험치에 대어 그리지 않는다.
    if (RewardPkt.has_level_up())
    {
        const Protocol::LevelUp& LevelUp = RewardPkt.level_up();
        ApplyLevel(LevelUp.level());
        ApplyStats(LevelUp.changed_stats());
    }

    ApplyStat(Protocol::STAT_TYPE_EXP, RewardPkt.exp());
    ApplyGold(RewardPkt.gold());
}
