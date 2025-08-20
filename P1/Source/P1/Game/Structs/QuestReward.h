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
    int32 Exp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Gold;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FItemData> Items;
};