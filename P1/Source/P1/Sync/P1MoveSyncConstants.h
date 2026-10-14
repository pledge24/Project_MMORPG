#pragma once

#include "CoreMinimal.h"

/** 원격 크리처의 이동 보정과 내 플레이어의 이동 패킷 송신이 함께 쓰는 상수. */
namespace P1MoveSync
{
    /** 이 거리(cm) 이상 벌어지면 보간하지 않고 서버 위치로 옮긴다. */
    constexpr float SNAP_DISTANCE = 800.f;

    /** 위치 보간 속도. FMath::VInterpTo에 넘긴다. */
    constexpr float LOCATION_INTERP_SPEED = 5.f;

    /** 회전 보간 속도. FMath::RInterpTo에 넘긴다. */
    constexpr float ROTATION_INTERP_SPEED = 5.f;

    /** 이동 패킷 전송 주기(초). */
    constexpr float MOVE_PACKET_SEND_DELAY = 0.2f;

    /** 원하는 이동 방향과 현재 회전의 차이가 이 각도(도) 이상이면 주기를 기다리지 않고 보낸다. */
    constexpr float YAW_TOLERANCE = 60.f;
}
