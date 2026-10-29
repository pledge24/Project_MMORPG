#pragma once

/**
 * 클라이언트의 아이템 요청(구매, 판매, 사용, 착용, 해제, 퀵슬롯 등록과 사용) 처리.
 *
 * 요청을 플레이어의 판정에 넘기고, 그 결과로 응답을 만들어 보낸다. 룸 상태는 읽기만 한다.
 * 모든 함수는 room의 큐 위에서 부른다.
 */
namespace ItemRequests
{
    /** 요청이 가리킨 NPC가 이 룸에 있고 거래할 수 있는 거리여야 하며, 그 NPC의 판매 목록에 있는 아이템만 판다. */
    void HandleBuyItem(Room& room, const PlayerRef& player, const Protocol::C_BUY_ITEM& pkt);
    /** 요청이 가리킨 NPC가 이 룸에 있고 거래할 수 있는 거리여야 한다. */
    void HandleSellItem(Room& room, const PlayerRef& player, const Protocol::C_SELL_ITEM& pkt);
    void HandleUseItem(Room& room, const PlayerRef& player, const Protocol::C_USE_ITEM& pkt);
    /** 성공하면 본인을 포함한 룸의 모든 플레이어에게 외형(부위와 템플릿)을 알린다. */
    void HandleEquipGear(Room& room, const PlayerRef& player, const Protocol::C_EQUIP_GEAR& pkt);
    /** 성공하면 본인을 포함한 룸의 모든 플레이어에게 외형(부위와 템플릿)을 알린다. */
    void HandleUnequipGear(Room& room, const PlayerRef& player, const Protocol::C_UNEQUIP_GEAR& pkt);
    void HandleSetQuickSlot(Room& room, const PlayerRef& player, const Protocol::C_SET_QUICK_SLOT& pkt);
    /** 응답은 소모품 사용과 같은 S_USE_ITEM이다. */
    void HandleUseQuickSlot(Room& room, const PlayerRef& player, const Protocol::C_USE_QUICK_SLOT& pkt);
}
