#include "Game/Progress/P1ItemActions.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/P1ItemData.h"
#include "Game/Inventory/P1InventorySlotAction.h"
#include "Game/Items/P1ItemContainer.h"
#include "Game/Items/P1ItemPackets.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Game/QuickSlots/P1QuickSlotPackets.h"
#include "Game/QuickSlots/P1QuickSlots.h"
#include "Network/P1PacketSender.h"
#include "Utils/LogCategory.h"

namespace
{
    bool IsInventorySection(EP1ContainerType Container)
    {
        return Container == EP1ContainerType::InventoryGear
            || Container == EP1ContainerType::InventoryConsumable
            || Container == EP1ContainerType::InventoryMisc;
    }

    const TCHAR* ToString(UP1ItemActions::EResult Result)
    {
        switch (Result)
        {
        case UP1ItemActions::EResult::Sent:          return TEXT("보냄");
        case UP1ItemActions::EResult::Empty:         return TEXT("빈 칸");
        case UP1ItemActions::EResult::Unusable:      return TEXT("할 수 없는 의도");
        case UP1ItemActions::EResult::LevelTooLow:   return TEXT("요구 레벨 미달");
        case UP1ItemActions::EResult::CoolingDown:   return TEXT("재사용 대기 중");
        case UP1ItemActions::EResult::NotSellable:   return TEXT("팔 수 없는 아이템");
        case UP1ItemActions::EResult::ShopClosed:    return TEXT("상점 창이 닫혀 있음");
        case UP1ItemActions::EResult::NotEnoughGold: return TEXT("골드 부족");
        case UP1ItemActions::EResult::Pending:       return TEXT("응답 대기 중");
        default:                                     return TEXT("알 수 없음");
        }
    }
}

UP1ItemActions::EResult UP1ItemActions::RequestForSlot(EP1ItemIntent Intent, const FP1SlotAddress& Address)
{
    FRequest Request;
    EResult Result = EResult::Unusable;
    switch (Intent)
    {
    case EP1ItemIntent::Use:     Result = DecideUse(Address, Request); break;
    case EP1ItemIntent::Unequip: Result = DecideUnequip(Address, Request); break;
    case EP1ItemIntent::Sell:    Result = DecideSell(Address, Request); break;
    default:                     break;
    }

    if (Result == EResult::Sent)
        Result = Send(Request);

    if (Result != EResult::Sent)
    {
        UE_LOG(LogP1CharacterInventory, Log, TEXT("아이템 요청을 보내지 않았다: %s. 의도 %d, 컨테이너 %d, 칸 %d"),
            ToString(Result), static_cast<int32>(Intent), static_cast<int32>(Address.Container), Address.SlotIndex);
    }
    return Result;
}

UP1ItemActions::EResult UP1ItemActions::RequestBuy(int32 TemplateId)
{
    const UP1MyPlayerData* MyPlayerData = GetMyPlayerData();
    const FP1ItemData* ItemData = TemplateId > 0 ? UP1GameDataSettings::FindItemData(TemplateId) : nullptr;

    EResult Result = EResult::Sent;
    if (MyPlayerData == nullptr || ItemData == nullptr)
        Result = EResult::Empty;
    else if (MyPlayerData->GetGold() < ItemData->BuyPrice)
        Result = EResult::NotEnoughGold;

    if (Result == EResult::Sent)
    {
        FRequest Request;
        Request.Kind = ERequestKind::Buy;
        Request.TemplateId = TemplateId;
        Request.Count = 1;
        Result = Send(Request);
    }

    if (Result != EResult::Sent)
        UE_LOG(LogP1CharacterInventory, Log, TEXT("구매 요청을 보내지 않았다: %s. 템플릿 %d"), ToString(Result), TemplateId);
    return Result;
}

UP1ItemActions::EResult UP1ItemActions::RequestSetQuickSlot(int32 QuickSlotIndex, const FP1QuickSlotRef& Ref)
{
    const UP1MyPlayerData* MyPlayerData = GetMyPlayerData();
    const UP1QuickSlots* QuickSlots = MyPlayerData ? MyPlayerData->GetQuickSlots() : nullptr;

    EResult Result = EResult::Sent;
    if (QuickSlots == nullptr || QuickSlots->Find(QuickSlotIndex) == nullptr)
    {
        Result = EResult::Empty;
    }
    else if (Ref.IsEmpty() == false)
    {
        // 서버도 소모품이 아닌 대상을 거절한다. 다 떨어진 소모품도 등록하므로 소지 여부는 보지 않는다.
        const FP1ItemData* ItemData = Ref.Type == EP1QuickSlotRefType::Item ? UP1GameDataSettings::FindItemData(Ref.Id) : nullptr;
        if (ItemData == nullptr || ItemData->GetItemType() != Protocol::ITEM_TYPE_CONSUMABLE)
            Result = EResult::Unusable;
    }

    if (Result == EResult::Sent)
    {
        FRequest Request;
        Request.Kind = ERequestKind::SetQuickSlot;
        Request.QuickSlotIndex = QuickSlotIndex;
        Request.QuickSlotRef = Ref;
        Result = Send(Request);
    }

    if (Result != EResult::Sent)
    {
        UE_LOG(LogP1QuickSlots, Log, TEXT("퀵슬롯 등록 요청을 보내지 않았다: %s. 퀵슬롯 %d, 종류 %d, 대상 %d"),
            ToString(Result), QuickSlotIndex, static_cast<int32>(Ref.Type), Ref.Id);
    }
    return Result;
}

const FP1Slot* UP1ItemActions::FindSlot(const FP1SlotAddress& Address) const
{
    const UP1MyPlayerData* MyPlayerData = GetMyPlayerData();
    const UP1ItemContainer* Container = MyPlayerData ? MyPlayerData->GetContainer(Address.Container) : nullptr;
    return Container ? Container->FindSlot(Address.SlotIndex) : nullptr;
}

UP1ItemActions::EResult UP1ItemActions::DecideUse(const FP1SlotAddress& Address, FRequest& OutRequest) const
{
    const FP1Slot* Slot_ = FindSlot(Address);
    if (Slot_ == nullptr || Slot_->HasItem() == false)
        return EResult::Empty;

    // 칸을 찾았으면 내 플레이어 데이터가 있다.
    const UP1MyPlayerData* MyPlayerData = GetMyPlayerData();

    // 정의가 없는 아이템은 종류를 몰라 쓸 수 없는 아이템으로 판정된다.
    const FP1ItemData* ItemData = UP1GameDataSettings::FindItemData(Slot_->Item.TemplateId);
    const Protocol::ItemType ItemType = ItemData ? ItemData->GetItemType() : Protocol::ITEM_TYPE_NONE;
    const int32 LevelRequirement = ItemData ? ItemData->LevelRequirement : 0;

    ERequestKind Kind;
    switch (FP1InventorySlotAction::Decide(Address.Container, ItemType, Slot_->Item.TemplateId, LevelRequirement,
        MyPlayerData->GetPlayerLevel(), MyPlayerData->IsItemCoolingDown(Slot_->Item.TemplateId)))
    {
    case FP1InventorySlotAction::EKind::Use:         Kind = ERequestKind::Use; break;
    case FP1InventorySlotAction::EKind::Equip:       Kind = ERequestKind::Equip; break;
    case FP1InventorySlotAction::EKind::Empty:       return EResult::Empty;
    case FP1InventorySlotAction::EKind::LevelTooLow: return EResult::LevelTooLow;
    case FP1InventorySlotAction::EKind::CoolingDown: return EResult::CoolingDown;
    default:                                         return EResult::Unusable;
    }

    OutRequest.Kind = Kind;
    OutRequest.Address = Address;
    OutRequest.ItemUid = Slot_->Item.ItemUid;
    return EResult::Sent;
}

UP1ItemActions::EResult UP1ItemActions::DecideUnequip(const FP1SlotAddress& Address, FRequest& OutRequest) const
{
    // 착용 장비 칸만 벗는다.
    if (Address.Container != EP1ContainerType::Equipped)
        return EResult::Unusable;

    const FP1Slot* Slot_ = FindSlot(Address);
    if (Slot_ == nullptr || Slot_->HasItem() == false)
        return EResult::Empty;

    OutRequest.Kind = ERequestKind::Unequip;
    OutRequest.Address = Address;
    OutRequest.ItemUid = Slot_->Item.ItemUid;
    return EResult::Sent;
}

UP1ItemActions::EResult UP1ItemActions::DecideSell(const FP1SlotAddress& Address, FRequest& OutRequest) const
{
    // 상점 창이 열려 있을 때 인벤토리 칸의 아이템만 판다. 착용한 장비는 벗은 뒤에 판다.
    if (bShopOpen == false)
        return EResult::ShopClosed;

    if (IsInventorySection(Address.Container) == false)
        return EResult::Unusable;

    const FP1Slot* Slot_ = FindSlot(Address);
    if (Slot_ == nullptr || Slot_->HasItem() == false)
        return EResult::Empty;

    // 서버도 팔 수 없는 아이템의 판매를 거절한다.
    const FP1ItemData* ItemData = UP1GameDataSettings::FindItemData(Slot_->Item.TemplateId);
    if (ItemData == nullptr || ItemData->Sellable == false)
        return EResult::NotSellable;

    // 판매는 한 번에 하나씩이다.
    OutRequest.Kind = ERequestKind::Sell;
    OutRequest.Address = Address;
    OutRequest.ItemUid = Slot_->Item.ItemUid;
    OutRequest.Count = 1;
    return EResult::Sent;
}

UP1ItemActions::EResult UP1ItemActions::Send(const FRequest& Request)
{
    if (IsPending(Request.Kind))
        return EResult::Pending;

    // 보내기 직전에 건다. 걸고 보내지 않으면 응답이 오지 않아 대기를 함께 쓰는 요청이 계속 막힌다.
    PendingGroups.Add(GetPendingGroup(Request.Kind));
    if (RequestSender)
        RequestSender(Request);
    else
        SendPacket(Request);
    return EResult::Sent;
}

void UP1ItemActions::SendPacket(const FRequest& Request) const
{
    // 게임 인스턴스를 넘기면 송신 창구가 월드를 거치지 않고 그 연결로 보낸다.
    const UP1MyPlayerData* MyPlayerData = GetMyPlayerData();
    const UObject* WorldContext = MyPlayerData ? static_cast<const UObject*>(MyPlayerData->GetGameInstance()) : this;

    switch (Request.Kind)
    {
    case ERequestKind::Use:
    {
        Protocol::C_USE_ITEM Pkt;
        *Pkt.mutable_address() = P1ItemPackets::ToProto(Request.Address);
        Pkt.set_item_uid(Request.ItemUid);
        FP1PacketSender::Send(WorldContext, Pkt);
        break;
    }
    case ERequestKind::Equip:
    {
        Protocol::C_EQUIP_GEAR Pkt;
        *Pkt.mutable_address() = P1ItemPackets::ToProto(Request.Address);
        Pkt.set_item_uid(Request.ItemUid);
        FP1PacketSender::Send(WorldContext, Pkt);
        break;
    }
    case ERequestKind::Unequip:
    {
        Protocol::C_UNEQUIP_GEAR Pkt;
        *Pkt.mutable_address() = P1ItemPackets::ToProto(Request.Address);
        Pkt.set_item_uid(Request.ItemUid);
        FP1PacketSender::Send(WorldContext, Pkt);
        break;
    }
    case ERequestKind::Sell:
    {
        Protocol::C_SELL_ITEM Pkt;
        *Pkt.mutable_address() = P1ItemPackets::ToProto(Request.Address);
        Pkt.set_item_uid(Request.ItemUid);
        Pkt.set_count(Request.Count);
        FP1PacketSender::Send(WorldContext, Pkt);
        break;
    }
    case ERequestKind::Buy:
    {
        Protocol::C_BUY_ITEM Pkt;
        Pkt.set_template_id(Request.TemplateId);
        Pkt.set_count(Request.Count);
        FP1PacketSender::Send(WorldContext, Pkt);
        break;
    }
    case ERequestKind::SetQuickSlot:
    {
        // 참조를 싣지 않으면 서버는 해제로 읽는다.
        Protocol::C_SET_QUICK_SLOT Pkt;
        Pkt.set_slot_index(Request.QuickSlotIndex);
        if (Request.QuickSlotRef.IsEmpty() == false)
            *Pkt.mutable_ref() = P1QuickSlotPackets::ToProto(Request.QuickSlotRef);
        FP1PacketSender::Send(WorldContext, Pkt);
        break;
    }
    }
}

UP1ItemActions::ERequestKind UP1ItemActions::GetPendingGroup(ERequestKind Kind)
{
    // 인벤토리 칸에 대한 요청은 응답 하나를 기다리는 동안 다음 요청을 보내지 않는다. 응답의 칸 변경이 오기 전에는
    // 클라이언트가 보는 칸이 서버의 칸과 다를 수 있다.
    switch (Kind)
    {
    case ERequestKind::Use:
    case ERequestKind::Equip:
    case ERequestKind::Sell:
        return ERequestKind::Use;
    default:
        return Kind;
    }
}

UP1MyPlayerData* UP1ItemActions::GetMyPlayerData() const
{
    return GetTypedOuter<UP1MyPlayerData>();
}
