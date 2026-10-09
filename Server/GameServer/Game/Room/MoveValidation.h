#pragma once
#include "Utils/Utils.h"

/**
 * 플레이어 이동 요청(C_MOVE)의 위치를 판정하는 자유 함수. 룸과 세션을 모른다.
 * 서버에는 지형이 없으므로 평면(x, y)의 이동 속도와 룸 경계만 본다. 높이(z)는 보지 않는다.
 */
namespace MoveValidation
{
    /** 플레이어의 최대 이동 속도(cm/s). 클라이언트 AP1Player의 MaxWalkSpeed와 같다. */
    constexpr float PLAYER_MAX_SPEED = 500.f;
    /** 속도에 곱하는 여유. 패킷 도착 간격의 흔들림을 흡수한다. */
    constexpr float SPEED_TOLERANCE = 1.5f;
    /** 경과 시간과 무관하게 더 허용하는 거리(cm). */
    constexpr float DISTANCE_TOLERANCE = 50.f;
    // 경과 시간에 상한을 두지 않는다. 버린 이동 뒤에 서버와 클라이언트의 위치가 벌어져도 시간이 흐르면 다시 받아들인다.
    // 대신 오래 서 있다가 보낸 첫 이동은 멀리 갈 수 있다(TD-048).

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
     * 통과하면 nullopt, 거절하면 로그에 남길 사유를 돌려준다.
     */
    optional<string> Validate(const vector2D& from, const vector2D& to, uint64 elapsedMs, const Bounds& bounds);
}
