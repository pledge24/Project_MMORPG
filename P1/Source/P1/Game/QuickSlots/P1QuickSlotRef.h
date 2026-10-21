#pragma once

#include "CoreMinimal.h"
#include "Game/QuickSlots/P1QuickSlotRefType.h"
#include "P1QuickSlotRef.generated.h"

/** 퀵슬롯이 가리키는 대상. 아이템 하나가 아니라 아이템 정의를 가리킨다. 기본값은 빈 참조다. */
USTRUCT(BlueprintType)
struct FP1QuickSlotRef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EP1QuickSlotRefType Type = EP1QuickSlotRefType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Id = 0;

    bool IsEmpty() const { return Type == EP1QuickSlotRefType::None; }

    static FP1QuickSlotRef ForItem(int32 TemplateId) { return FP1QuickSlotRef{ EP1QuickSlotRefType::Item, TemplateId }; }

    bool operator==(const FP1QuickSlotRef&) const = default;
};
