#include "Core/pch.h"
#include "Game/Action/ActionValidation.h"

optional<ActionRejection> ActionValidation::ValidateYaw(float yaw)
{
    // NaN은 어느 비교도 통과하지 못하므로 범위 검사 전에 거른다. 그대로 두면 공격 범위의 회전이 NaN이 되어 아무도 맞지 않는다.
    if (std::isfinite(yaw) == false || yaw < -180.f || yaw > 180.f)
        return ActionRejection{ Protocol::COMBAT_RESULT_INVALID_YAW, format("시작 방향 {}이 -180~180도 밖이다", yaw) };

    return nullopt;
}

optional<ActionRejection> ActionValidation::ValidateCombo(int32 requestedActionId, int32 firstActionId, const optional<LastAction>& last)
{
    // 직업의 첫 단계와 NextAction은 액션 표에 있고 기동할 때 판정 데이터와 대조했다. 없으면 표를 바꿔 끼운 테스트다.
    if (Gamedata::FindAction(requestedActionId) == nullptr || Gamedata::FindActionTiming(requestedActionId) == nullptr)
        return ActionRejection{ Protocol::COMBAT_RESULT_INVALID_ACTION, format("액션 {}이 액션 표나 크리처 판정 데이터에 없다", requestedActionId) };

    // 직전 액션은 시작할 때 액션 표와 판정 데이터에 있었다. 없으면 표를 바꿔 끼운 테스트이므로 직전 액션이 없던 것으로 본다.
    const DesignData::ActionRow* lastAction = last.has_value() ? Gamedata::FindAction(last->actionId) : nullptr;
    const ActionTiming* lastTiming = last.has_value() ? Gamedata::FindActionTiming(last->actionId) : nullptr;
    if (lastAction == nullptr || lastTiming == nullptr)
    {
        if (requestedActionId != firstActionId)
            return ActionRejection{ Protocol::COMBAT_RESULT_INVALID_ACTION, format("직전 액션 없이 콤보 첫 단계 {}가 아닌 액션 {}을 요청했다", firstActionId, requestedActionId) };

        return nullopt;
    }

    // 다음 단계는 직전 액션이 끝날 때까지 잇는다. 입력 창이 섹션 끝 직전에 닫히는 단계가 있어, 지연만큼 늦게 온 요청도 받는다.
    const float elapsed = last->elapsed;
    const bool continuesCombo = lastAction->NextAction == requestedActionId && elapsed - LATENCY_TOLERANCE <= lastTiming->length;
    if (requestedActionId != firstActionId && continuesCombo == false)
    {
        return ActionRejection{ Protocol::COMBAT_RESULT_INVALID_ACTION, format("액션 {}은 콤보 첫 단계 {}도, 직전 액션 {}({}초 지남)의 다음 단계도 아니다",
            requestedActionId, firstActionId, last->actionId, elapsed) };
    }

    // 직전 액션이 끝나기 전에는 그 액션의 콤보 입력 창이 열린 뒤에만 새 액션을 시작한다. 입력 창이 없는 액션은 끝까지 끊을 수 없다.
    if (elapsed + LATENCY_TOLERANCE >= lastTiming->length)
        return nullopt;

    const optional<ComboWindow>& window = lastTiming->comboWindow;
    if (window.has_value() == false || elapsed + LATENCY_TOLERANCE < window->start)
    {
        return ActionRejection{ Protocol::COMBAT_RESULT_TOO_EARLY, format("직전 액션 {}이 {}초 지났고 콤보 입력 창이 {}초에 열리기 전이다",
            last->actionId, elapsed, window.has_value() ? window->start : lastTiming->length) };
    }

    return nullopt;
}
