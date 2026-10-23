#pragma once

#include "CoreMinimal.h"

/** 월드의 혼잡도. 인증 서버가 전달한 게임 서버의 단계이고, 월드 선택을 막지 않는다. */
enum class EP1WorldCongestion : uint8
{
    Smooth,
    Normal,
    Crowded,
};
