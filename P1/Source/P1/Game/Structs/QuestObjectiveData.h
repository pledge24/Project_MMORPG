#pragma once

#include "CoreMinimal.h"
#include "QuestObjectiveData.generated.h"

/*----------------------
    QuestObjectiveData
-----------------------*/

USTRUCT(BlueprintType)
struct FQuestObjectiveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Type;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TemplateId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Count;
};