#pragma once

#include "CoreMinimal.h"
#include "P1CharacterClass.generated.h"

/** 캐릭터의 직업. 값이 Protocol::CharacterClass와 같다. */
UENUM()
enum class EP1CharacterClass : uint8
{
    None = 0,
    Warrior = 1,
    Mage = 2,
};
