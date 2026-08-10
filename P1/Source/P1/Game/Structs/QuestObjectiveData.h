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
    int32 TemplateId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Count = 0;
};