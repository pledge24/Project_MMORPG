#include "Core/pch.h"
#include "Network/ItemRequests.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"
#include "Network/ItemPackets.h"

namespace
{
    // 착용과 해제의 응답. 본인에게는 바뀐 칸과 스탯까지, 다른 플레이어에게는 외형만 보낸다.
    template<typename ResponseType>
    void SendGearChange(Room& room, const PlayerRef& player, ResponseType& response, optional<GearChangeResult>& result)
    {
        const int64 entityId = player->GetEntityId();
        response.set_entity_id(entityId);
        response.set_success(result.has_value());

        if (result.has_value() == false)
        {
            SendPacket(player->GetSession(), response);
            return;
        }

        // gear_type과 template_id는 다른 플레이어에게 보일 외형(장비 부위와 그 부위의 아이템)이다.
        response.set_gear_type(result->gearType);
        response.set_template_id(result->templateId);
        SendBufferRef othersBuffer = ServerPacketHandler::MakeSerializedPacket(response);

        ItemPackets::AddChangedSlots(result->changedSlots, response.mutable_changed_slots());
        *response.mutable_updated_stat() = std::move(result->updatedStats);
        SendPacket(player->GetSession(), response);

        room.Broadcast(othersBuffer, entityId);
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
    optional<BuyItemResult> result = player->ProcessBuyItem(pkt.template_id());
    buyItemPkt.set_success(result.has_value());
    if (result.has_value())
    {
        ItemPackets::AddChangedSlots(result->changedSlots, buyItemPkt.mutable_changed_slots());
        buyItemPkt.set_gold(result->gold);
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
    optional<SellItemResult> result = player->ProcessSellItem(ItemPackets::ToRequest(pkt.address(), pkt.item_uid()));
    sellItemPkt.set_success(result.has_value());
    if (result.has_value())
    {
        ItemPackets::AddChangedSlots(result->changedSlots, sellItemPkt.mutable_changed_slots());
        sellItemPkt.set_gold(result->gold);
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

    // 거부 응답에도 싣는다. 클라이언트는 이 id로 내 플레이어를 찾은 뒤에야 요청 대기를 푼다.
    Protocol::S_USE_ITEM useItemPkt;
    useItemPkt.set_entity_id(player->GetEntityId());

    optional<UseItemResult> result = player->ProcessUseItem(ItemPackets::ToRequest(pkt.address(), pkt.item_uid()), ::GetTickCount64());
    useItemPkt.set_success(result.has_value());
    if (result.has_value())
    {
        ItemPackets::AddChangedSlots(result->changedSlots, useItemPkt.mutable_changed_slots());
        *useItemPkt.mutable_updated_stat() = std::move(result->updatedStats);
    }

    SendPacket(session, useItemPkt);
}

void ItemRequests::HandleEquipGear(Room& room, const PlayerRef& player, const Protocol::C_EQUIP_GEAR& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    Protocol::S_EQUIP_GEAR equipGearPkt;
    optional<GearChangeResult> result = player->ProcessEquipGear(ItemPackets::ToRequest(pkt.address(), pkt.item_uid()));
    SendGearChange(room, player, equipGearPkt, result);
}

void ItemRequests::HandleUnequipGear(Room& room, const PlayerRef& player, const Protocol::C_UNEQUIP_GEAR& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    Protocol::S_UNEQUIP_GEAR unequipGearPkt;
    optional<GearChangeResult> result = player->ProcessUnequipGear(ItemPackets::ToRequest(pkt.address(), pkt.item_uid()));
    SendGearChange(room, player, unequipGearPkt, result);
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
    setQuickSlotPkt.set_success(false);

    // 열거형에 없는 종류는 판정에 넘기지 않고 거절한다. 참조가 없으면 해제 요청이다.
    optional<QuickSlotRef> ref;
    if (pkt.has_ref())
    {
        ref = ItemPackets::FromProto(pkt.ref());
        if (ref.has_value() == false)
        {
            SendPacket(session, setQuickSlotPkt);
            return;
        }
    }

    optional<SetQuickSlotResult> result = player->ProcessSetQuickSlot(pkt.slot_index(), ref);
    setQuickSlotPkt.set_success(result.has_value());
    if (result.has_value())
        ItemPackets::AddQuickSlots(result->changedQuickSlots, setQuickSlotPkt.mutable_changed_quick_slots());

    SendPacket(session, setQuickSlotPkt);
}
