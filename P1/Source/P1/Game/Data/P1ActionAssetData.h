#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "P1ActionAssetData.generated.h"

class UAnimMontage;

/** 액션의 표현 데이터. 행 이름은 액션 번호이고, 기획 원본에서 오지 않고 에디터에서만 편집한다. */
USTRUCT(BlueprintType)
struct P1_API FP1ActionAssetData : public FTableRowBase
{
    GENERATED_BODY()

    /** 액션이 재생하는 몽타주. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSoftObjectPtr<UAnimMontage> Montage;

    /** 몽타주에서 재생할 섹션. 섹션 하나가 액션 하나다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    FName Section;
};
