#pragma once

#include "CharacterOverview.generated.h"

/*--------------------
   CharacterOverview
--------------------*/

USTRUCT(BlueprintType)
struct FCharacterOverview
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
