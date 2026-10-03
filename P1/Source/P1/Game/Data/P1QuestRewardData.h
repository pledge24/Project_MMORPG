#pragma once

#include "CoreMinimal.h"
#include "Game/Data/P1ItemData.h"
#include "P1QuestRewardData.generated.h"

USTRUCT(BlueprintType)
struct FP1QuestRewardData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Exp = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Gold = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FP1ItemData> Items;
};