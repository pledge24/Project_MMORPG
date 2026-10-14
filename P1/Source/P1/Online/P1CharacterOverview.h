#pragma once

#include "P1CharacterOverview.generated.h"

/** 캐릭터 선택 화면에 보여 줄 캐릭터 하나의 요약. */
USTRUCT(BlueprintType)
struct FP1CharacterOverview
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "CharacterSelect")
    int64 CharacterId = -1;

    UPROPERTY(BlueprintReadWrite, Category = "CharacterSelect")
    FString CharacterClass;

    UPROPERTY(BlueprintReadWrite, Category = "CharacterSelect")
    FString CharacterName;

    UPROPERTY(BlueprintReadWrite, Category = "CharacterSelect")
    int32 CharacterLevel = 0;
};
