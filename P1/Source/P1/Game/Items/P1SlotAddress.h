#pragma once

#include "CoreMinimal.h"
#include "Game/Items/P1ContainerType.h"
#include "P1SlotAddress.generated.h"

/** 칸 하나를 가리키는 주소. 착용 장비에서는 칸 번호가 장비 부위 값이다. */
USTRUCT(BlueprintType)
struct FP1SlotAddress
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EP1ContainerType Container = EP1ContainerType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SlotIndex = 0;

    bool operator==(const FP1SlotAddress&) const = default;
};
