#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MapData.generated.h"

/*------------------
       MapData
-------------------*/

USTRUCT(BlueprintType)
struct FMapData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TemplateId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MapName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MapType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool HaveStartPoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MapPath;

};