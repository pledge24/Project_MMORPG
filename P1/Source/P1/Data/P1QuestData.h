#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Data/P1QuestObjectiveData.h"
#include "Data/P1QuestRewardData.h"
#include "P1QuestData.generated.h"

/*------------------
      QuestData
-------------------*/

USTRUCT(BlueprintType)
struct FP1QuestData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Name = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TemplateId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LevelRequirement = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString QuestType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StartNpc = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EndNpc = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FP1QuestObjectiveData Objectives;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FP1QuestRewardData Rewards;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool Repeatable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;
};