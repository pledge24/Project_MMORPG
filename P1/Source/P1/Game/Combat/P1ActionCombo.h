#pragma once

#include "CoreMinimal.h"

/**
 * 내 플레이어의 일반 공격 콤보 규칙.
 *
 * 공격 입력, 콤보 입력 창, 섹션 끝을 받아 어느 액션을 언제 시작할지 정한다. 재생과 패킷 전송은 부르는 쪽이 한다.
 * 다음 단계는 콤보 입력 창 안에서 누른 입력으로만 이어지고, 창이 닫힐 때 시작한다.
 */
struct P1_API FP1ActionCombo
{
    /** 액션 번호로 콤보의 다음 단계 액션을 찾는다. 다음 단계가 없으면 0. */
    using FFindNextAction = TFunction<int32(int32)>;

    /** 다음 단계를 찾는 함수가 없으면 콤보가 첫 단계에서 끝난다. */
    explicit FP1ActionCombo(FFindNextAction InFindNextAction = nullptr);

    /**
     * 공격 입력. 진행 중인 액션이 없으면 FirstActionId를 시작하고 그 번호를 돌려준다.
     * 진행 중이면 입력 창이 열려 있을 때만 다음 단계를 예약하고, 어느 경우든 0을 돌려준다.
     */
    int32 Input(int32 FirstActionId);

    void BeginWindow();
    /** 입력 창이 닫혔다. 예약한 다음 단계가 있으면 그 액션을 시작하고 번호를 돌려준다. 없으면 0. */
    int32 EndWindow();

    /** 진행 중인 액션의 섹션이 다음 단계 없이 끝났다. 다음 입력은 첫 단계부터다. */
    void EndAction();

    /** 진행 중인 액션이 없으면 0. */
    int32 GetCurrentAction() const { return CurrentAction; }
    bool IsActing() const { return CurrentAction != 0; }

private:
    FFindNextAction FindNextAction;

    int32 CurrentAction = 0;
    /** 입력 창 안에서 예약한 다음 단계. 없으면 0. */
    int32 QueuedAction = 0;
    bool bWindowOpen = false;
};
