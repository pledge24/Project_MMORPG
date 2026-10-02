#pragma once

#include "CoreMinimal.h"

/** 이동 동기화의 상수다. 원격 크리처의 보정과 내 플레이어의 이동 패킷 송신이 함께 쓴다. 이 파일 밖에 두지 않는다. */
namespace P1MoveSync
{
    /** 이 거리(cm) 이상 벌어지면 보간하지 않고 서버 위치로 옮긴다. */
    constexpr float SNAP_DISTANCE = 800.f;

    /** FMath::VInterpTo에 넘기는 보간 속도다. */
    constexpr float LOCATION_INTERP_SPEED = 5.f;

    /** FMath::RInterpTo에 넘기는 보간 속도다. */
    constexpr float ROTATION_INTERP_SPEED = 5.f;

    /** 초 단위 이동 패킷 전송 주기다. */
    constexpr float MOVE_PACKET_SEND_DELAY = 0.2f;

    /** 원하는 이동 방향과 현재 회전의 차이가 이 각도(도) 이상이면 주기를 기다리지 않고 보낸다. */
    constexpr float YAW_TOLERANCE = 60.f;
}
