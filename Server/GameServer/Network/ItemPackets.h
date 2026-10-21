#pragma once
#include "Game/Items/ItemTypes.h"

struct ItemSlotRequest;

/**
 * 서버의 아이템 타입과 패킷 사이의 변환.
 *
 * 아이템과 칸의 Protocol 타입은 이 변환에서만 다룬다. 입장 패킷의 소지품과 응답의 바뀐 칸을 만들고, 요청의 칸을 읽는다.
 */
namespace ItemPackets
{
    Protocol::Item ToProto(const ItemInstance& item);
    ItemInstance FromProto(const Protocol::Item& item);

    Protocol::SlotType ToSlotType(ContainerType container);
    /** 컨테이너가 아닌 칸 종류면 ContainerType::NONE. 클라이언트 값을 넘겨도 된다. */
    ContainerType ToContainerType(Protocol::SlotType slotType);

    /** 응답에 실을 바뀐 칸. 비게 된 칸은 REMOVED로 표시한다. 클라이언트가 그 표시로만 칸을 비운다. */
    Protocol::Slot ToChangedSlot(const SlotChange& change);
    void AddChangedSlots(const vector<SlotChange>& changes, OUT RepeatedPtrField<Protocol::Slot>* slots);

    /** 요청 패킷의 칸을 읽는다. 값을 검증하지 않으므로 판정은 플레이어가 한다. */
    ItemSlotRequest ToRequest(const Protocol::Slot& slot);

    /** 입장 패킷에 실을 골드, 인벤토리의 모든 칸, 착용 장비의 모든 칸. */
    Protocol::Possession MakePossession(const Player& player);
}
