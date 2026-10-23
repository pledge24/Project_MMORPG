#pragma once
#include "Game/Items/ItemTypes.h"
#include "Game/QuickSlots/QuickSlotTypes.h"

struct ItemSlotRequest;

/**
 * 서버의 아이템 타입과 패킷 사이의 변환.
 *
 * 아이템과 칸, 퀵슬롯의 Protocol 타입은 이 변환에서만 다룬다. 입장 패킷의 소지품과 응답의 칸 변경을 만들고, 요청의 칸 주소를 읽는다.
 */
namespace ItemPackets
{
    Protocol::Item ToProto(const ItemInstance& item);

    Protocol::ContainerType ToProto(ContainerType container);
    /** 열거형에 없는 값이면 ContainerType::NONE. 클라이언트 값을 넘겨도 된다. */
    ContainerType FromProto(Protocol::ContainerType container);

    Protocol::SlotAddress ToProto(const SlotAddress& address);
    /** 값을 검증하지 않는다. 클라이언트 값을 넘겨도 된다. */
    SlotAddress FromProto(const Protocol::SlotAddress& address);

    /** 응답에 실을 칸 변경. 비게 된 칸은 아이템을 싣지 않는다. */
    Protocol::SlotChange ToProto(const SlotChange& change);
    /** 칸 변경마다 패킷의 칸 변경 하나를 protoChanges 뒤에 붙인다(nullptr 불가). */
    void AddChangedSlots(const vector<SlotChange>& changes, OUT RepeatedPtrField<Protocol::SlotChange>* protoChanges);

    /** 요청 패킷이 가리킨 칸을 읽는다. 값을 검증하지 않으므로 판정은 플레이어가 한다. */
    ItemSlotRequest ToRequest(const Protocol::SlotAddress& address, int64 itemUid);

    /** 입장 패킷에 실을 골드와, 인벤토리의 구역과 착용 장비의 모든 칸. */
    Protocol::Possession MakePossession(const Player& player);

    /** 빈 퀵슬롯은 참조를 싣지 않는다. */
    Protocol::QuickSlot ToProto(const QuickSlot& quickSlot);
    /** 퀵슬롯마다 패킷의 퀵슬롯 하나를 protoQuickSlots 뒤에 붙인다(nullptr 불가). */
    void AddQuickSlots(const vector<QuickSlot>& quickSlots, OUT RepeatedPtrField<Protocol::QuickSlot>* protoQuickSlots);
    /** 열거형에 없는 종류면 nullopt. 대상은 검증하지 않으므로 클라이언트 값을 넘겨도 된다. */
    optional<QuickSlotRef> FromProto(const Protocol::QuickSlotRef& ref);
}
