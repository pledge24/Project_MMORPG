#pragma once

#include "CoreMinimal.h"

#define USING_SHARED_PTR(name)	using name##Ref = TSharedPtr<class name>;

USING_SHARED_PTR(Session);
USING_SHARED_PTR(PacketSession);
USING_SHARED_PTR(SendBuffer);
USING_SHARED_PTR(RecvWorker);
USING_SHARED_PTR(SendWorker);



