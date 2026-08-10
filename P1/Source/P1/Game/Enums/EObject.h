#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EObjectType : uint8
{
    None UMETA(DisplayName = "None"),
    Monster UMETA(DisplayName = "Monster"),
    Player UMETA(DisplayName = "Player"),
    Projectile UMETA(DisplayName = "Projectile"),
};