#include "Core/pch.h"
#include "Network/ItemRequests.h"
#include "Game/Entities/Npc.h"
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

    // 상점 요청이 가리킨 NPC가 이 룸에 있고 플레이어가 그 NPC와 거래할 수 있는 거리인지 본다. 거래할 수 없으면 nullptr.
    // 다른 룸의 NPC 번호는 이 룸에서 찾지 못한다. 클라이언트의 화면에서 NPC가 사라진 뒤에 온 요청도 여기서 걸린다.
    NpcRef FindReachableNpc(Room& room, const PlayerRef& player, int64 npcEntityId)
    {
        NpcRef npc = room.FindNpc(npcEntityId);
        if (npc == nullptr || npc->IsWithinReach(player->GetPosInfo()) == false)
            return nullptr;

        return npc;
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

void ItemRequests::C_HandleBuyItem(Room& room, const PlayerRef& player, const Protocol::C_BUY_ITEM& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    auto session = player->GetSession();
    if (session == nullptr)
        return;

    Protocol::S_BUY_ITEM buyItemPkt;
    NpcRef npc = FindReachableNpc(room, player, pkt.npc_entity_id());
    if (npc == nullptr)
    {
        buyItemPkt.set_result(Protocol::ITEM_RESULT_NPC_OUT_OF_REACH);
        SendPacket(session, buyItemPkt);
        return;
    }

    // 상점 창은 판매 목록의 아이템만 진열하므로 정상 클라이언트는 목록 밖의 아이템을 사지 않는다.
    if (npc->Sells(pkt.template_id()) == false)
    {
        buyItemPkt.set_result(Protocol::ITEM_RESULT_INVALID_REQUEST);
        SendPacket(session, buyItemPkt);
        return;
    }

    ItemOutcome<BuyItemResult> outcome = player->ProcessBuyItem(pkt.template_id());
    buyItemPkt.set_result(outcome.GetResult());
    if (outcome.IsSuccess())
    {
        ItemPackets::AddChangedSlots(outcome->changedSlots, buyItemPkt.mutable_changed_slots());
        buyItemPkt.set_gold(outcome->gold);
    }

    SendPacket(session, buyItemPkt);
}

void ItemRequests::C_HandleSellItem(Room& room, const PlayerRef& player, const Protocol::C_SELL_ITEM& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    auto session = player->GetSession();
    if (session == nullptr)
        return;

    Protocol::S_SELL_ITEM sellItemPkt;
    // 어느 상점이든 팔 수 있는 아이템이면 사들인다. 거리만 본다.
    if (FindReachableNpc(room, player, pkt.npc_entity_id()) == nullptr)
    {
        sellItemPkt.set_result(Protocol::ITEM_RESULT_NPC_OUT_OF_REACH);
        SendPacket(session, sellItemPkt);
        return;
    }

    ItemOutcome<SellItemResult> outcome = player->ProcessSellItem(ItemPackets::ToRequest(pkt.address(), pkt.item_uid()));
    sellItemPkt.set_result(outcome.GetResult());
    if (outcome.IsSuccess())
    {
        ItemPackets::AddChangedSlots(outcome->changedSlots, sellItemPkt.mutable_changed_slots());
        sellItemPkt.set_gold(outcome->gold);
    }

    SendPacket(session, sellItemPkt);
}

void ItemRequests::C_HandleUseItem(Room& room, const PlayerRef& player, const Protocol::C_USE_ITEM& pkt)
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

void ItemRequests::C_HandleEquipGear(Room& room, const PlayerRef& player, const Protocol::C_EQUIP_GEAR& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    ItemOutcome<GearChangeResult> outcome = player->ProcessEquipGear(ItemPackets::ToRequest(pkt.address(), pkt.item_uid()));
    SendGearChange<Protocol::S_EQUIP_GEAR>(room, player, outcome);
}

void ItemRequests::C_HandleUnequipGear(Room& room, const PlayerRef& player, const Protocol::C_UNEQUIP_GEAR& pkt)
{
    // 잡이 기다리는 사이 룸을 떠났으면 처리하지 않는다. 그 플레이어의 상태는 이제 새 룸 큐의 것이다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    ItemOutcome<GearChangeResult> outcome = player->ProcessUnequipGear(ItemPackets::ToRequest(pkt.address(), pkt.item_uid()));
    SendGearChange<Protocol::S_UNEQUIP_GEAR>(room, player, outcome);
}

void ItemRequests::C_HandleSetQuickSlot(Room& room, const PlayerRef& player, const Protocol::C_SET_QUICK_SLOT& pkt)
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

void ItemRequests::C_HandleUseQuickSlot(Room& room, const PlayerRef& player, const Protocol::C_USE_QUICK_SLOT& pkt)
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
