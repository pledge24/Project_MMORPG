#pragma once

#include "CoreMinimal.h"
#include "P1QuestRewardItemData.generated.h"

/** 퀘스트 보상으로 받는 아이템 하나. */
USTRUCT(BlueprintType)
struct FP1QuestRewardItemData
{
    GENERATED_BODY()

    /** 아이템 번호. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Id = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity = 0;
};
