#pragma once

#include "CoreMinimal.h"

/** 서버가 받는 채팅의 한도. 서버의 검사와 같은 수를 쓴다. */
namespace P1ChatLimits
{
    /** 메시지 최대 길이(UTF-16 기준). */
    constexpr int32 MAX_MESSAGE_LENGTH = 100;
}
