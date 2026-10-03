#pragma once

#include "CoreMinimal.h"

/** 내 플레이어의 이동 패킷을 이번 프레임에 보낼지와 다음 프레임에 쓸 타이머다. */
struct P1_API FP1MoveSendThrottle
{
    bool bSend = false;

    /** 초 단위다. 보냈으면 전송 주기로 돌아간다. */
    float NextTimer = 0.f;

    /** 한 프레임 분량의 판정에 넣는 값이다. */
    struct FInput
    {
        /** 초 단위다. */
        float RemainingTimer = 0.f;

        /** 초 단위다. */
        float DeltaSeconds = 0.f;

        bool bInputChanged = false;
        bool bCanInputMovement = false;
        float DesiredYaw = 0.f;
        float CurrentYaw = 0.f;
        bool bAttacking = false;
    };

    /**
     * 한 프레임 분량의 판정이다.
     * 주기가 끝나면 보낸다. 이동 입력이 가능할 때 입력이 바뀌었거나, 원하는 이동 Yaw와 현재 Yaw의 차이가
     * 허용치 이상이면 주기를 기다리지 않고 보낸다. 공격 중에는 주기를 기다리지 않는 송신만 막는다.
     */
    static FP1MoveSendThrottle Decide(const FInput& In);
};
