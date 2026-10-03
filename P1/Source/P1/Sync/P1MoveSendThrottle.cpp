#include "Sync/P1MoveSendThrottle.h"
#include "Sync/P1MoveSyncConstants.h"

FP1MoveSendThrottle FP1MoveSendThrottle::Decide(const FInput& In)
{
    bool bForceSend = In.bInputChanged && In.bCanInputMovement;

    // 입력이 있으면 몸이 돌아갈 방향을, 없으면 서버가 마지막으로 받은 방향을 현재 방향과 비교한다.
    // 입력을 떼면 원하는 이동 Yaw가 0이 되므로 그 값은 쓰지 않는다. 차이는 ±180도에서 감싸서 잰다.
    const float CompareYaw = In.bHasMoveInput ? In.DesiredYaw : In.LastSentYaw;
    if (FMath::Abs(FMath::FindDeltaAngleDegrees(In.CurrentYaw, CompareYaw)) >= P1MoveSync::YAW_TOLERANCE)
        bForceSend = true;

    // 공격 중에는 바로 보내지 않는다. 주기 송신은 막지 않는다.
    if (In.bAttacking)
        bForceSend = false;

    FP1MoveSendThrottle Decision;
    Decision.NextTimer = In.RemainingTimer - In.DeltaSeconds;

    if (Decision.NextTimer <= 0.f || bForceSend)
    {
        Decision.bSend = true;
        Decision.NextTimer = P1MoveSync::MOVE_PACKET_SEND_DELAY;
    }

    return Decision;
}
