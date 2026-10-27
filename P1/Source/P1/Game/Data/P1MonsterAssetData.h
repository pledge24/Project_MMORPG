#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "P1MonsterAssetData.generated.h"

/** 몬스터의 에셋 참조. 행 이름은 TemplateId이고, 기획 원본에서 오지 않고 에디터에서만 편집한다. */
USTRUCT(BlueprintType)
struct P1_API FP1MonsterAssetData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftClassPtr<class AP1Monster> MonsterClass;
};
