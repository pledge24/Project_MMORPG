#pragma once

#include "CoreMinimal.h"
#include "Structs/ItemData.h"
#include "SlotData.generated.h"

/*-----------------
      ItemData
-----------------*/

USTRUCT(BlueprintType)
struct FSlotData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SlotId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FItemData Item;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity;
};