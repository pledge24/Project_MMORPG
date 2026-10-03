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

        /** 이번 프레임에 이동 입력이 있으면 true다. 입력을 떼면 원하는 이동 Yaw가 0이 되므로 비교에 쓰지 않는다. */
        bool bHasMoveInput = false;

        /** 도 단위다. 아래 두 yaw도 같다. ±180도 밖의 값이어도 감싸서 잰다. */
        float DesiredYaw = 0.f;

        float CurrentYaw = 0.f;

        /** 서버와 마지막으로 맞춘 내 플레이어의 yaw다. 입력이 없을 때 현재 yaw와 비교한다. */
        float LastSyncedYaw = 0.f;

        bool bAttacking = false;
    };

    /**
     * 한 프레임 분량의 판정이다.
     * 주기가 끝나면 보낸다. 이동 입력이 가능할 때 입력이 바뀌었으면 주기를 기다리지 않고 보낸다.
     * 입력이 있으면 원하는 이동 Yaw와, 없으면 LastSyncedYaw와 현재 Yaw의 차이가 허용치 이상일 때도 바로 보낸다.
     * 각도 차이는 ±180도에서 감싸서 잰다. 공격 중에는 주기를 기다리지 않는 송신만 막는다.
     */
    static FP1MoveSendThrottle Decide(const FInput& In);
};
