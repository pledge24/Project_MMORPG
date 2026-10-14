#pragma once

#include "CoreMinimal.h"

/**
 * 내 플레이어의 이동 패킷 송신 판정.
 *
 * 프레임마다 이동 패킷을 지금 보낼지와 다음 주기 송신까지 남은 시간을 정한다.
 */
struct P1_API FP1MoveSendThrottle
{
    /** 이번 프레임에 보내면 true. */
    bool bSend = false;

    /** 다음 주기 송신까지 남은 시간(초). */
    float NextTimer = 0.f;

    /** 한 프레임의 판정 입력. */
    struct FInput
    {
        /** 다음 주기 송신까지 남은 시간(초). */
        float RemainingTimer = 0.f;

        float DeltaSeconds = 0.f;

        bool bInputChanged = false;
        bool bCanInputMovement = false;

        /** 이번 프레임에 이동 입력이 있으면 true. 없으면 DesiredYaw를 쓰지 않는다. */
        bool bHasMoveInput = false;

        /** 원하는 이동 방향(도). 아래 두 yaw도 도 단위이고, ±180도 밖이어도 된다. */
        float DesiredYaw = 0.f;

        float CurrentYaw = 0.f;

        /** 서버와 마지막으로 맞춘 yaw. 이동 입력이 없을 때 현재 yaw와 비교한다. */
        float LastSyncedYaw = 0.f;

        bool bAttacking = false;
    };

    /**
     * 한 프레임 분량의 송신 판정. 주기가 끝났거나 입력이나 방향이 크게 바뀌었으면 보낸다.
     * 공격 중에는 주기 송신만 한다.
     */
    static FP1MoveSendThrottle Decide(const FInput& In);
};
