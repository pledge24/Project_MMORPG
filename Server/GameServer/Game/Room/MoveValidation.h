#pragma once
#include "Utils/PlaneBounds.h"
#include "Utils/Utils.h"

class NavQuery;

/**
 * 플레이어 이동 요청의 판정 규칙.
 *
 * 클라이언트가 보낸 이동이 이동 속도, 룸 경계, 지형 안에서 가능한지 판정한다.
 * 지형으로는 벽을 통과했는지와 바닥에서 떨어진 위치인지를 본다. 위치의 z는 캡슐 중심이다.
 */
namespace MoveValidation
{
    /** 플레이어의 최대 이동 속도(cm/s). 클라이언트 AP1Player의 MaxWalkSpeed와 같다. */
    constexpr float PLAYER_MAX_SPEED = 500.f;
    /** 속도에 곱하는 여유. 패킷 도착 간격의 흔들림을 흡수한다. */
    constexpr float SPEED_TOLERANCE = 1.5f;
    /** 경과 시간과 무관하게 더 허용하는 거리(cm). */
    constexpr float DISTANCE_TOLERANCE = 50.f;
    /** 표면을 따라 간 끝과 도착 위치 사이에 허용하는 평면 거리(cm). 반경만큼 깎인 navmesh 가장자리를 조금 벗어난 캡슐을 받아들인다. */
    constexpr float WALL_TOLERANCE = 30.f;
    /** 캡슐 중심과 바닥 위 캡슐 중심 높이 사이에 허용하는 차이(cm). 단차를 오르는 중의 차이보다 크다. */
    constexpr float HEIGHT_TOLERANCE = 100.f;

    /**
     * from(직전에 받아들인 위치)에서 elapsedMs 뒤에 to로 옮기는 요청을 판정한다. 통과하면 nullopt, 거절하면 로그에 남길 사유.
     * terrain이 nullptr이면 지형을 보지 않는다. terrain은 룸 큐 위에서만 쓴다.
     */
    optional<string> Validate(const vector3D& from, const vector3D& to, uint64 elapsedMs, const PlaneBounds& bounds, NavQuery* terrain);
}
