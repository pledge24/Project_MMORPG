#pragma once

#include "CoreMinimal.h"
#include "QuestReward.generated.h"

/*----------------------
       QuestReward
-----------------------*/

USTRUCT(BlueprintType)
struct FQuestRewardData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Exp = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Gold = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FItemData> Items;
};