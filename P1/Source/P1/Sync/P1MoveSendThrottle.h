#pragma once

#include "CoreMinimal.h"

/** 이번 프레임에 이동 패킷을 보낼지와 다음 프레임에 쓸 타이머다. */
struct FP1MoveSendDecision
{
    bool bSend = false;

    /** 초 단위다. 보냈으면 전송 주기로 돌아간다. */
    float NextTimer = 0.f;
};

/** 내 플레이어의 이동 패킷을 언제 보낼지 정한다. */
struct P1_API FP1MoveSendThrottle
{
    /**
     * 한 프레임 분량의 판정이다. RemainingTimer와 DeltaSeconds는 초 단위다.
     * 주기가 끝나면 보낸다. 이동 입력이 가능할 때 입력이 바뀌었거나, 원하는 이동 Yaw와 현재 Yaw의 차이가
     * 허용치 이상이면 주기를 기다리지 않고 보낸다. 공격 중에는 주기를 기다리지 않는 송신만 막는다.
     */
    static FP1MoveSendDecision Decide(
        float RemainingTimer,
        float DeltaSeconds,
        bool bInputChanged,
        bool bCanInputMovement,
        float DesiredYaw,
        float CurrentYaw,
        bool bAttacking);
};
