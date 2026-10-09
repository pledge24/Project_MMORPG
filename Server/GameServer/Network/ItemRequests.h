#pragma once

/**
 * 아이템 요청 다섯 개(구매, 판매, 사용, 착용, 해제)의 처리. 패킷 핸들러가 플레이어의 소속 룸 큐에 넣고,
 * 그 큐 위에서 실행된다. 판정은 Player의 Process* 함수가 하고, 여기서는 결과로 응답 패킷을 만들어 보낸다.
 * 룸 상태는 읽기만 한다(플레이어가 아직 룸에 있는지, 다른 플레이어에게 보낼 외형).
 * 잡이 기다리는 사이 세션이 끊겼으면 응답을 보내지 않는다.
 */
namespace ItemRequests
{
    void HandleBuyItem(const PlayerRef& player, const Protocol::C_BUY_ITEM& pkt);
    void HandleSellItem(const PlayerRef& player, const Protocol::C_SELL_ITEM& pkt);
    void HandleUseItem(const PlayerRef& player, const Protocol::C_USE_ITEM& pkt);
    /** 성공하면 같은 룸의 다른 플레이어에게 외형(부위와 템플릿)만 알린다. */
    void HandleEquipGear(Room& room, const PlayerRef& player, const Protocol::C_EQUIP_GEAR& pkt);
    /** 성공하면 같은 룸의 다른 플레이어에게 외형(부위와 템플릿)만 알린다. */
    void HandleUnequipGear(Room& room, const PlayerRef& player, const Protocol::C_UNEQUIP_GEAR& pkt);
}
