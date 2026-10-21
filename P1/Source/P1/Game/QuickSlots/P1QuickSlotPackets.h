#pragma once

#include "CoreMinimal.h"
#include "Protocol.pb.h"
#include "Game/QuickSlots/P1QuickSlot.h"
#include "Game/QuickSlots/P1QuickSlotRef.h"

/**
 * 클라이언트의 퀵슬롯 타입과 패킷 사이의 변환.
 *
 * 퀵슬롯의 Protocol 타입은 이 변환에서만 다룬다. 입장 패킷과 등록 응답의 퀵슬롯을 읽고, 등록 요청의 참조를 만든다.
 */
namespace P1QuickSlotPackets
{
    /** 참조가 없거나 열거형에 없는 종류면 빈 퀵슬롯이 된다. */
    FP1QuickSlot FromProto(const Protocol::QuickSlot& QuickSlot);

    Protocol::QuickSlotRef ToProto(const FP1QuickSlotRef& Ref);
}
