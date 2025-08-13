#pragma once
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "BP_Structs.generated.h" // <- 헤더 파일 이름에 맞게 변경

USTRUCT(BlueprintType)
struct FCharacterOverview
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "CharacterSelect")
    int64 CharacterId;

    UPROPERTY(BlueprintReadWrite, Category = "CharacterSelect")
    FString CharacterClass;

    UPROPERTY(BlueprintReadWrite, Category = "CharacterSelect")
    FString CharacterName;

    UPROPERTY(BlueprintReadWrite, Category = "CharacterSelect")
    int32 CharacterLevel;
};