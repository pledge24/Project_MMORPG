#pragma once

#include "CoreMinimal.h"

/** 클라이언트가 구분하는 엔티티 종류. */
UENUM(BlueprintType)
enum class EP1EntityType : uint8
{
    None UMETA(DisplayName = "None"),
    Monster UMETA(DisplayName = "Monster"),
    Player UMETA(DisplayName = "Player"),
    Projectile UMETA(DisplayName = "Projectile"),
};