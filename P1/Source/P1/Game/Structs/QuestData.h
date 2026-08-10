#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "QuestObjectiveData.h"
#include "QuestReward.h"
#include "QuestData.generated.h"

/*------------------
      QuestData
-------------------*/

USTRUCT(BlueprintType)
struct FQuestData : public FTableRowBase
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
    FQuestObjectiveData Objectives;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FQuestRewardData Rewards;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool Repeatable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;
};