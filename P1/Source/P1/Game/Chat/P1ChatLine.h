#pragma once

#include "CoreMinimal.h"
#include "P1ChatLine.generated.h"

/** 채팅 한 줄을 누가 남겼는가. 화면이 이름의 색을 고르는 기준이다. */
UENUM(BlueprintType)
enum class EP1ChatLineKind : uint8
{
    Mine,
    Other,
    /** 이름이 없는 안내 문구. 서버가 메시지를 거절했을 때 내 화면에만 남는다. */
    System,
};

/** 채팅창에 남는 한 줄. */
USTRUCT(BlueprintType)
struct FP1ChatLine
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EP1ChatLineKind Kind = EP1ChatLineKind::Other;

    /** 말한 플레이어의 이름. 안내 문구이거나 서버가 이름을 싣지 않았으면 비어 있다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Message;

    bool operator==(const FP1ChatLine&) const = default;
};
