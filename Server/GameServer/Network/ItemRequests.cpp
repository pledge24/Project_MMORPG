#include "Core/pch.h"
#include "Network/ItemRequests.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"

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

        // slot_id와 template_id는 요청 슬롯이 아니라 처리 결과(장비 부위와 그 부위의 아이템)다.
        response.set_slot_id(result->gearType);
        response.set_template_id(result->templateId);
        SendBufferRef othersBuffer = ServerPacketHandler::MakeSerializedPacket(response);

        *response.mutable_updated_slots() = std::move(result->updatedSlots);
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
        *buyItemPkt.mutable_updated_slots() = std::move(result->updatedSlots);
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
    optional<SellItemResult> result = player->ProcessSellItem(pkt.slot());
    sellItemPkt.set_success(result.has_value());
    if (result.has_value())
    {
        *sellItemPkt.mutable_updated_slot() = std::move(result->updatedSlot);
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

    // 슬롯과 스탯은 성공했을 때만 싣는다.
    optional<UseItemResult> result = player->ProcessUseItem(pkt.slot(), ::GetTickCount64());
    useItemPkt.set_success(result.has_value());
    if (result.has_value())
    {
        *useItemPkt.mutable_updated_slots()->Add() = std::move(result->updatedSlot);
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
    optional<GearChangeResult> result = player->ProcessEquipGear(pkt.slot());
    SendGearChange(room, player, equipGearPkt, result);
}

void ItemRequests::HandleUnequipGear(Room& room, const PlayerRef& player, const Protocol::C_UNEQUIP_GEAR& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    Protocol::S_UNEQUIP_GEAR unequipGearPkt;
    optional<GearChangeResult> result = player->ProcessUnequipGear(pkt.slot());
    SendGearChange(room, player, unequipGearPkt, result);
}
