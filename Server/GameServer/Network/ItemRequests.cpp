#include "Core/pch.h"
#include "Network/ItemRequests.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"
#include "Network/ItemPackets.h"

namespace
{
    // 착용과 해제의 응답. 본인에게는 바뀐 칸과 스탯을 보내고, 외형은 본인을 포함한 룸의 모든 플레이어에게 알린다.
    template<typename ResponseType>
    void SendGearChange(Room& room, const PlayerRef& player, ItemOutcome<GearChangeResult>& outcome)
    {
        auto session = player->GetSession();
        if (session == nullptr)
            return;

        ResponseType response;
        response.set_result(outcome.GetResult());
        if (outcome.IsSuccess() == false)
        {
            SendPacket(session, response);
            return;
        }

        ItemPackets::AddChangedSlots(outcome->changedSlots, response.mutable_changed_slots());
        *response.mutable_changed_stats() = std::move(outcome->changedStats);
        SendPacket(session, response);

        // 외형은 요청한 인벤토리 칸이 아니라 장착된 장비 부위로 알린다. 지금 보지 않는 플레이어는 나중에 스폰으로 외형을 받는다.
        Protocol::S_GEAR_APPEARANCE appearancePkt;
        appearancePkt.set_entity_id(player->GetEntityId());
        appearancePkt.set_gear_part(outcome->gearPart);
        appearancePkt.set_template_id(outcome->templateId);
        room.BroadcastNearby(player->GetEntityId(), ServerPacketHandler::MakeSerializedPacket(appearancePkt));
    }

    // 소모품 사용의 응답. 칸을 골라 쓴 요청과 퀵슬롯으로 쓴 요청이 같은 응답을 받는다.
    void SendUseItem(const GameSessionRef& session, ItemOutcome<UseItemResult>& outcome)
    {
        Protocol::S_USE_ITEM useItemPkt;
        useItemPkt.set_result(outcome.GetResult());
        if (outcome.IsSuccess())
        {
            ItemPackets::AddChangedSlots(outcome->changedSlots, useItemPkt.mutable_changed_slots());
            *useItemPkt.mutable_changed_stats() = std::move(outcome->changedStats);
        }

        SendPacket(session, useItemPkt);
    }
}

void ItemRequests::HandleBuyItem(Room& room, const PlayerRef& player, const Protocol::C_BUY_ITEM& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    auto session = player->GetSession();
    if (session == nullptr)
        return;

    Protocol::S_BUY_ITEM buyItemPkt;
    ItemOutcome<BuyItemResult> outcome = player->ProcessBuyItem(pkt.template_id());
    buyItemPkt.set_result(outcome.GetResult());
    if (outcome.IsSuccess())
    {
        ItemPackets::AddChangedSlots(outcome->changedSlots, buyItemPkt.mutable_changed_slots());
        buyItemPkt.set_gold(outcome->gold);
    }

    SendPacket(session, buyItemPkt);
}

void ItemRequests::HandleSellItem(Room& room, const PlayerRef& player, const Protocol::C_SELL_ITEM& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    auto session = player->GetSession();
    if (session == nullptr)
        return;

    Protocol::S_SELL_ITEM sellItemPkt;
    ItemOutcome<SellItemResult> outcome = player->ProcessSellItem(ItemPackets::ToRequest(pkt.address(), pkt.item_uid()));
    sellItemPkt.set_result(outcome.GetResult());
    if (outcome.IsSuccess())
    {
        ItemPackets::AddChangedSlots(outcome->changedSlots, sellItemPkt.mutable_changed_slots());
        sellItemPkt.set_gold(outcome->gold);
    }

    SendPacket(session, sellItemPkt);
}

void ItemRequests::HandleUseItem(Room& room, const PlayerRef& player, const Protocol::C_USE_ITEM& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    auto session = player->GetSession();
    if (session == nullptr)
        return;

    ItemOutcome<UseItemResult> outcome = player->ProcessUseItem(ItemPackets::ToRequest(pkt.address(), pkt.item_uid()), ::GetTickCount64());
    SendUseItem(session, outcome);
}

void ItemRequests::HandleEquipGear(Room& room, const PlayerRef& player, const Protocol::C_EQUIP_GEAR& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    ItemOutcome<GearChangeResult> outcome = player->ProcessEquipGear(ItemPackets::ToRequest(pkt.address(), pkt.item_uid()));
    SendGearChange<Protocol::S_EQUIP_GEAR>(room, player, outcome);
}

void ItemRequests::HandleUnequipGear(Room& room, const PlayerRef& player, const Protocol::C_UNEQUIP_GEAR& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    ItemOutcome<GearChangeResult> outcome = player->ProcessUnequipGear(ItemPackets::ToRequest(pkt.address(), pkt.item_uid()));
    SendGearChange<Protocol::S_UNEQUIP_GEAR>(room, player, outcome);
}

void ItemRequests::HandleSetQuickSlot(Room& room, const PlayerRef& player, const Protocol::C_SET_QUICK_SLOT& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    auto session = player->GetSession();
    if (session == nullptr)
        return;

    Protocol::S_SET_QUICK_SLOT setQuickSlotPkt;

    // 열거형에 없는 종류는 판정에 넘기지 않고 거절한다. 참조가 없으면 해제 요청이다.
    optional<QuickSlotRef> ref;
    if (pkt.has_ref())
    {
        ref = ItemPackets::FromProto(pkt.ref());
        if (ref.has_value() == false)
        {
            setQuickSlotPkt.set_result(Protocol::ITEM_RESULT_INVALID_REQUEST);
            SendPacket(session, setQuickSlotPkt);
            return;
        }
    }

    ItemOutcome<SetQuickSlotResult> outcome = player->ProcessSetQuickSlot(pkt.quick_slot_index(), ref);
    setQuickSlotPkt.set_result(outcome.GetResult());
    if (outcome.IsSuccess())
        ItemPackets::AddQuickSlots(outcome->changedQuickSlots, setQuickSlotPkt.mutable_changed_quick_slots());

    SendPacket(session, setQuickSlotPkt);
}

void ItemRequests::HandleUseQuickSlot(Room& room, const PlayerRef& player, const Protocol::C_USE_QUICK_SLOT& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    auto session = player->GetSession();
    if (session == nullptr)
        return;

    ItemOutcome<UseItemResult> outcome = player->ProcessUseQuickSlot(pkt.quick_slot_index(), ::GetTickCount64());
    SendUseItem(session, outcome);
}
