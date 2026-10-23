#pragma once

#include "CoreMinimal.h"
#include "Auth/P1WorldCongestion.h"

/** 월드 목록의 항목. 접속 주소는 월드를 고른 뒤에 받으므로 싣지 않는다. */
struct FP1World
{
    int32 WorldId = 0;
    FString Name;
    EP1WorldCongestion Congestion = EP1WorldCongestion::Smooth;
};
