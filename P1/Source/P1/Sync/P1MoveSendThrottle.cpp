#include "Sync/P1MoveSendThrottle.h"
#include "Sync/P1MoveSyncConstants.h"

FP1MoveSendThrottle FP1MoveSendThrottle::Decide(const FInput& In)
{
    // 이동 입력이 가능할 때 입력이 바뀌었으면 주기를 기다리지 않고 보낸다.
    bool bForceSend = In.bInputChanged && In.bCanInputMovement;

    // 방향 차이가 허용치 이상이어도 바로 보낸다. 입력이 있으면 원하는 이동 Yaw와, 없으면 서버와 마지막으로
    // 맞춘 yaw와 비교한다. 입력을 떼면 원하는 이동 Yaw가 0이 되므로 입력이 없을 때는 쓰지 않는다.
    const float CompareYaw = In.bHasMoveInput ? In.DesiredYaw : In.LastSyncedYaw;
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
