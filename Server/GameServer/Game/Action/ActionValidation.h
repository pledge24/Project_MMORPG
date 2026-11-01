#pragma once
#include "Protocol/Combat.pb.h"

/** 크리처가 마지막으로 시작한 액션. 액션이 끝난 뒤에도 남는다. */
struct LastAction
{
    int32 actionId = 0;
    /** 액션을 시작한 뒤 흐른 초. 룸 틱으로 센다. */
    float elapsed = 0.f;
};

/** 액션 요청을 거절한 결과와 로그에 남길 사유. */
struct ActionRejection
{
    Protocol::CombatResult result = Protocol::COMBAT_RESULT_SUCCESS;
    string reason;
};

/**
 * 플레이어 액션 요청의 판정 규칙.
 *
 * 클라이언트가 보낸 액션이 콤보의 순서와 콤보 입력 창의 시점에 맞는지, 시작 방향이 올바른 값인지 판정한다.
 * 액션 표와 크리처 판정 데이터는 Gamedata에서 찾는다. 사망과 시작 위치는 부르는 쪽이 판정한다.
 */
namespace ActionValidation
{
    /** 콤보 시점 판정에 앞뒤로 허용하는 초. 클라이언트의 재생과 서버의 룸 틱 사이에 생기는 네트워크 지연의 흔들림을 흡수한다. */
    constexpr float LATENCY_TOLERANCE = 0.2f;

    /** yaw(도)가 -180도에서 180도 사이의 유한한 수면 nullopt. */
    optional<ActionRejection> ValidateYaw(float yaw);

    /**
     * firstActionId(직업의 콤보 첫 단계)로 시작하는 콤보에서 requestedActionId를 지금 시작해도 되는지 판정한다.
     * last는 그 크리처가 마지막으로 시작한 액션이고, 시작한 적이 없으면 nullopt. 통과하면 nullopt.
     */
    optional<ActionRejection> ValidateCombo(int32 requestedActionId, int32 firstActionId, const optional<LastAction>& last);
}
