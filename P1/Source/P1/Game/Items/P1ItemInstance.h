#pragma once

#include "CoreMinimal.h"
#include "P1ItemInstance.generated.h"

/** 칸에 든 아이템 하나. 묶음으로 쌓이는 아이템이면 묶음 하나다. */
USTRUCT(BlueprintType)
struct FP1ItemInstance
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 ItemUid = 0;

    /** 0 이하이면 아이템이 없다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TemplateId = 0;

    /** 묶음의 개수. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Count = 0;

    bool operator==(const FP1ItemInstance&) const = default;
};
