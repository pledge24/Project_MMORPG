#pragma once

#include "CoreMinimal.h"
#include "Game/Items/P1ItemInstance.h"
#include "Game/Items/P1SlotAddress.h"
#include "P1Slot.generated.h"

/** 칸 하나의 주소와 지금 내용. 빈 칸이면 Item이 기본값이다. */
USTRUCT(BlueprintType)
struct FP1Slot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FP1SlotAddress Address;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FP1ItemInstance Item;

    bool HasItem() const { return Item.TemplateId > 0; }

    bool operator==(const FP1Slot&) const = default;
};
