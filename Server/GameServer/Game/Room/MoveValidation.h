#pragma once
#include "Utils/Utils.h"

/**
 * 플레이어 이동 요청의 판정 규칙.
 *
 * 클라이언트가 보낸 이동이 이동 속도와 룸 경계 안에서 가능한지 판정한다.
 * 서버에는 지형이 없으므로 평면만 보고 높이는 보지 않는다.
 */
namespace MoveValidation
{
    /** 플레이어의 최대 이동 속도(cm/s). 클라이언트 AP1Player의 MaxWalkSpeed와 같다. */
    constexpr float PLAYER_MAX_SPEED = 500.f;
    /** 속도에 곱하는 여유. 패킷 도착 간격의 흔들림을 흡수한다. */
    constexpr float SPEED_TOLERANCE = 1.5f;
    /** 경과 시간과 무관하게 더 허용하는 거리(cm). */
    constexpr float DISTANCE_TOLERANCE = 50.f;

    /** 룸의 평면 경계. 경계 위의 점은 안쪽이다. */
    struct Bounds
    {
        float minX = 0.f;
        float maxX = 0.f;
        float minY = 0.f;
        float maxY = 0.f;
    };

    /**
     * from(직전에 받아들인 위치)에서 elapsedMs 뒤에 to로 옮기는 요청을 판정한다.
     * 통과하면 nullopt, 거절하면 로그에 남길 사유.
     */
    optional<string> Validate(const vector2D& from, const vector2D& to, uint64 elapsedMs, const Bounds& bounds);
}
