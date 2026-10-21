#pragma once

#include "CoreMinimal.h"
#include "Game/Items/P1SlotAddress.h"
#include "UI/Common/P1SlotDisplay.h"

/** 끌고 있는 슬롯이 그리던 자리와 내용. 놓는 쪽의 위젯이 이 값으로 요청을 정한다. */
struct FP1SlotDragPayload
{
    /** 끌기를 시작한 슬롯이 그리던 것. */
    enum class EOrigin : uint8
    {
        ContainerSlot,
        QuickSlot,
    };

    EOrigin Origin = EOrigin::ContainerSlot;

    /** Origin이 ContainerSlot일 때 끌어 온 칸. */
    FP1SlotAddress Address;

    /** Origin이 QuickSlot일 때 끌어 온 퀵슬롯 번호. */
    int32 QuickSlotIndex = 0;

    /** 끌기를 시작할 때 그 슬롯이 그리던 내용. */
    FP1SlotDisplay Display;
};
