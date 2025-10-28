#pragma once

#include "CoreMinimal.h"
#include "BP_Structs.h"

#define USING_SHARED_PTR(name)	using name##Ref = TSharedPtr<class name>;

USING_SHARED_PTR(Session);
USING_SHARED_PTR(PacketSession);
USING_SHARED_PTR(SendBuffer);
USING_SHARED_PTR(RecvWorker);
USING_SHARED_PTR(SendWorker);

UENUM(BlueprintType)
enum class ECreatureType
{
    None UMETA(DisplayName = "None"),
    Monster UMETA(DisplayName = "Monster"),
    Player UMETA(DisplayName = "Player"),
};

