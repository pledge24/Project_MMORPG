#pragma once

#include "CoreMinimal.h"
#include "Game/QuickSlots/P1QuickSlotRef.h"
#include "P1QuickSlot.generated.h"

/** 퀵슬롯 하나의 번호와 지금 대상. 빈 퀵슬롯이면 Ref가 빈 참조다. */
USTRUCT(BlueprintType)
struct FP1QuickSlot
{
    GENERATED_BODY()

    /** 0부터 퀵슬롯 개수 미만. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SlotIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FP1QuickSlotRef Ref;

    bool operator==(const FP1QuickSlot&) const = default;
};
