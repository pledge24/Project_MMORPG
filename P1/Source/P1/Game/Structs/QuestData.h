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
    int32 Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TemplateId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LevelRequirement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString QuestType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StartNpc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EndNpc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FQuestObjectiveData Objectives;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FQuestRewardData Rewards;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool Repeatable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;
};