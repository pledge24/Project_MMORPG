#pragma once

#include "CoreMinimal.h"
#include "Protocol.pb.h"
#include "Game/Items/P1ContainerType.h"
#include "Game/Items/P1ItemInstance.h"
#include "Game/Items/P1Slot.h"
#include "Game/Items/P1SlotAddress.h"

/**
 * 클라이언트의 아이템 타입과 패킷 사이의 변환.
 *
 * 아이템과 칸의 Protocol 타입은 이 변환에서만 다룬다. 입장 패킷의 소지품과 응답의 칸 변경을 읽고, 요청의 칸 주소를 만든다.
 */
namespace P1ItemPackets
{
    Protocol::ContainerType ToProto(EP1ContainerType Container);
    /** 열거형에 없는 값이면 EP1ContainerType::None. */
    EP1ContainerType FromProto(Protocol::ContainerType Container);

    Protocol::SlotAddress ToProto(const FP1SlotAddress& Address);
    FP1SlotAddress FromProto(const Protocol::SlotAddress& Address);

    FP1ItemInstance FromProto(const Protocol::Item& Item);

    /** 아이템이 없는 칸 변경은 빈 칸이 된다. */
    FP1Slot FromProto(const Protocol::SlotChange& Change);

    /** 컨테이너 하나의 모든 칸을 패킷에 실린 순서대로. */
    TArray<FP1Slot> FromProto(const Protocol::ContainerSlots& ContainerSlots);
}
