#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "P1ItemAssetData.generated.h"

/** 아이템의 에셋 참조다. 행 이름은 TemplateId다. 기획 원본에서 오지 않고 에디터에서만 편집한다. */
USTRUCT(BlueprintType)
struct FP1ItemAssetData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UStaticMesh> StaticMesh;
};
