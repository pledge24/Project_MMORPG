#pragma once

#include "CoreMinimal.h"
#include "P1QuestObjectiveData.generated.h"

USTRUCT(BlueprintType)
struct FP1QuestObjectiveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Type;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TemplateId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Count = 0;
};