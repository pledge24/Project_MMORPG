#include "Game/Progress/P1MyPlayerData.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/P1ItemData.h"
#include "Game/Inventory/P1Inventory.h"
#include "Game/Equipment/P1EquippedGear.h"
#include "Game/Entities/P1MyPlayer.h"
#include "Utils/LogCategory.h"

void UP1MyPlayerData::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 인벤토리 객체를 만든다
    Inventory = NewObject<UP1Inventory>(this, UP1Inventory::StaticClass());
    if (Inventory == nullptr)
        UE_LOG(LogP1CharacterComp, Warning, TEXT("Inventory Is Not Exist"));

    // 장비 객체를 만든다
    EquippedGear = NewObject<UP1EquippedGear>(this, UP1EquippedGear::StaticClass());
    if (EquippedGear == nullptr)
        UE_LOG(LogP1CharacterComp, Warning, TEXT("EquippedGear Is Not Exist"));

    // 프로토콜 사본
    _EntityInfo = MakeUnique<Protocol::EntityInfo>();
    _PlayerInfo = _EntityInfo->mutable_player_info();
    _StatInfo = MakeUnique<Protocol::StatInfo>();
    _Possession = MakeUnique<Protocol::Possession>();

    // 스탯마다 변경 델리게이트 자리를 만든다(공통)
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MAX_HP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_HP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MAX_MP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_PHYSICAL_ATTACK);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MAGICAL_ATTACK);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MAX_EXP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_EXP);

    // 델리게이트를 바인딩한다
    OnMyPlayerSpawned.AddUObject(this, &UP1MyPlayerData::BindMyPlayerDelegate);

    // 소지품 델리게이트는 이 서브시스템과 함께 사는 객체끼리 잇는다. 플레이어 액터와 무관하므로 한 번만 붙인다.
    // 내 플레이어가 스폰될 때마다 붙이면 맵을 옮길 때마다 핸들러가 하나씩 늘어난다.
    OnGoldChanged.AddUObject(this, &UP1MyPlayerData::Rep_GoldChanged);
    OnInvenSlotChanged.AddUObject(Inventory, &UP1Inventory::Rep_SlotChanged);
    OnEquipmentSlotChanged.AddUObject(EquippedGear, &UP1EquippedGear::Rep_SlotChanged);
}

void UP1MyPlayerData::Deinitialize()
{
    Inventory = nullptr;
    EquippedGear = nullptr;

    // _PlayerInfo는 _EntityInfo 안을 가리키므로 먼저 끊는다.
    _PlayerInfo = nullptr;
    _EntityInfo.Reset();
    _StatInfo.Reset();
    _Possession.Reset();

    Super::Deinitialize();
}

void UP1MyPlayerData::InitMyPlayerData(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
    // 저장
    {
        SetEntityInfo(EnterGamePkt.player());
        _StatInfo->CopyFrom(EnterGamePkt.stat_info());
        _Possession->CopyFrom(EnterGamePkt.possession());
        
        // 자주 읽는 값을 캐시한다
        _PlayerId = _EntityInfo->entity_id();
        _PlayerName = FText::FromString(UTF8_TO_TCHAR(_EntityInfo->player_info().name().c_str()));
    }

    // 소지품 래퍼를 초기화한다
    {
        Inventory->Init(_Possession->mutable_inventory());
        EquippedGear->Init(_Possession->mutable_equipped_gear());
    }

    // 서버는 재접속하면 재사용 대기를 잊는다. 같은 시점에 비워 둘의 판정을 맞춘다.
    ItemCooldowns.Reset();

}

void UP1MyPlayerData::BindMyPlayerDelegate(AP1MyPlayer* MyPlayer)
{
    if (IsValid(MyPlayer) == false)
    {
        UE_LOG(LogP1CharacterComp, Warning, TEXT("MyPlayer Is InValid"));
        return;
    }

    // 내 플레이어 액터는 맵마다 새로 스폰되므로 액터의 델리게이트는 스폰 때마다 붙인다.
    MyPlayer->OnLevelUp.AddUObject(this, &UP1MyPlayerData::Rep_LevelChanged);
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
    // 프로토버프는 메세지를 CopyFrom할때마다 필트의 포인터가 달라질 수 있다.
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
    // 알리기만 하고 사본을 두면 안 된다. HUD는 생성될 때 사본을 읽으므로 맵을 옮긴 뒤 옛 값이 보인다.
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

void UP1MyPlayerData::Rep_GoldChanged(const int64 Gold) const
{
    _Possession->set_gold(Gold);
}

void UP1MyPlayerData::Rep_LevelChanged(int32 Level) const
{
    _PlayerInfo->set_level(Level);
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

    for (const Protocol::Slot& UpdatedSlot : BuyItemPkt.updated_slots())
    {
        OnInvenSlotChanged.Broadcast(UpdatedSlot, false);
    }
    OnGoldChanged.Broadcast(BuyItemPkt.gold());
}

void UP1MyPlayerData::HandleSellItem(const Protocol::S_SELL_ITEM& SellItemPkt)
{
    OnRecvSellItemPkt.Broadcast();
    if (SellItemPkt.success() == false)
        return;

    OnInvenSlotChanged.Broadcast(SellItemPkt.updated_slot(), false);
    OnGoldChanged.Broadcast(SellItemPkt.gold());
}

void UP1MyPlayerData::HandleUseItem(const Protocol::S_USE_ITEM& UseItemPkt)
{
    if (IsMyPlayer(UseItemPkt.entity_id()) == false)
        return;

    OnRecvUseItemPkt.Broadcast();
    if (UseItemPkt.success() == false)
        return;

    // 마지막 한 개를 쓰면 응답의 슬롯에 아이템이 없다. 사본이 바뀌기 전에 쓴 아이템의 템플릿을 읽는다.
    TArray<int32, TInlineAllocator<1>> UsedTemplateIds;
    for (const Protocol::Slot& UpdatedSlot : UseItemPkt.updated_slots())
    {
        const Protocol::Slot* Before = Inventory->FindSlot(UpdatedSlot.type(), UpdatedSlot.slot_id());
        if (Before && Before->has_item())
            UsedTemplateIds.Add(Before->item().template_id());
    }

    for (const Protocol::Slot& UpdatedSlot : UseItemPkt.updated_slots())
    {
        OnInvenSlotChanged.Broadcast(UpdatedSlot, true);
    }
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

    ApplyGearSlots(EquipGearPkt.updated_slots());
    ApplyStats(EquipGearPkt.updated_stat());
}

void UP1MyPlayerData::HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt)
{
    if (IsMyPlayer(UnequipGearPkt.entity_id()) == false)
        return;

    OnRecvUnequipGearPkt.Broadcast();
    if (UnequipGearPkt.success() == false)
        return;

    ApplyGearSlots(UnequipGearPkt.updated_slots());
    ApplyStats(UnequipGearPkt.updated_stat());
}

void UP1MyPlayerData::ApplyGearSlots(const google::protobuf::RepeatedPtrField<Protocol::Slot>& UpdatedSlots)
{
    for (const Protocol::Slot& UpdatedSlot : UpdatedSlots)
    {
        switch (UpdatedSlot.type())
        {
        case Protocol::SLOT_TYPE_EQUIPPED:
            OnEquipmentSlotChanged.Broadcast(UpdatedSlot);
            break;

        case Protocol::SLOT_TYPE_INVENTORY_GEAR:
        case Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE:
        case Protocol::SLOT_TYPE_INVENTORY_MISC:
            OnInvenSlotChanged.Broadcast(UpdatedSlot, false);
            break;
        }
    }
}
