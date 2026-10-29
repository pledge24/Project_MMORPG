#pragma once

#include "CoreMinimal.h"
#include "P1ChannelEntry.generated.h"

/** 채널 목록의 한 줄. 인원은 서버가 목록을 만든 순간의 값이다. */
USTRUCT()
struct FP1ChannelEntry
{
    GENERATED_BODY()

    /** 1부터다. */
    UPROPERTY()
    int32 Channel = 0;

    UPROPERTY()
    int32 PlayerCount = 0;
};
