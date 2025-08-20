#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MonsterData.generated.h"

/*------------------
     MonsterData
-------------------*/

USTRUCT(BlueprintType)
struct FMonsterData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TemplateId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Level;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MonsterName;
};