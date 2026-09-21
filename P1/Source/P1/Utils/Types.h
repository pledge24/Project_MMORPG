#pragma once

#include "CoreMinimal.h"
#include "Data/BP_Structs.h"
#include "Entities/P1EntityType.h"

#define USING_SHARED_PTR(name)	using name##Ref = TSharedPtr<class name>;

USING_SHARED_PTR(Session);
USING_SHARED_PTR(PacketSession);
USING_SHARED_PTR(SendBuffer);
USING_SHARED_PTR(FP1RecvWorker);
USING_SHARED_PTR(FP1SendWorker);