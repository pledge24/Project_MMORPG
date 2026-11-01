#include "Game/Combat/P1ActionCombo.h"

FP1ActionCombo::FP1ActionCombo(FFindNextAction InFindNextAction)
    : FindNextAction(MoveTemp(InFindNextAction))
{
}

int32 FP1ActionCombo::Input(int32 FirstActionId)
{
    if (CurrentAction == 0)
    {
        CurrentAction = FirstActionId;
        QueuedAction = 0;
        bWindowOpen = false;
        return CurrentAction;
    }

    // 창 밖의 입력과 이미 예약한 뒤의 입력은 버린다. 입력을 쌓아 두면 창이 닫힌 뒤에 누른 입력으로도 콤보가 이어진다.
    if (bWindowOpen && QueuedAction == 0)
        QueuedAction = FindNextAction ? FindNextAction(CurrentAction) : 0;

    return 0;
}

void FP1ActionCombo::BeginWindow()
{
    if (CurrentAction != 0)
        bWindowOpen = true;
}

int32 FP1ActionCombo::EndWindow()
{
    bWindowOpen = false;
    if (QueuedAction == 0)
        return 0;

    CurrentAction = QueuedAction;
    QueuedAction = 0;
    return CurrentAction;
}

void FP1ActionCombo::EndAction()
{
    CurrentAction = 0;
    QueuedAction = 0;
    bWindowOpen = false;
}
