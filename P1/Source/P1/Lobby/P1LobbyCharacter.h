#pragma once

#include "CoreMinimal.h"
#include "Lobby/P1CharacterClass.h"
#include "P1LobbyCharacter.generated.h"

/** 로비의 캐릭터 목록에 있는 캐릭터 하나. */
USTRUCT()
struct FP1LobbyCharacter
{
    GENERATED_BODY()

    UPROPERTY()
    int64 CharacterId = -1;

    UPROPERTY()
    FString CharacterName;

    UPROPERTY()
    EP1CharacterClass CharacterClass = EP1CharacterClass::None;

    /** 마지막으로 저장한 레벨. */
    UPROPERTY()
    int32 CharacterLevel = 0;
};
