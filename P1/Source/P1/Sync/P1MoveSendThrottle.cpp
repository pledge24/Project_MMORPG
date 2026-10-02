#include "Sync/P1MoveSendThrottle.h"
#include "Sync/P1MoveSyncConstants.h"

FP1MoveSendDecision FP1MoveSendThrottle::Decide(
    float RemainingTimer,
    float DeltaSeconds,
    bool bInputChanged,
    bool bCanInputMovement,
    float DesiredYaw,
    float CurrentYaw,
    bool bAttacking)
{
    bool bForceSend = bInputChanged && bCanInputMovement;

    // 각도를 감싸지 않는다. 179도와 -179도도 358도 차이로 보고 바로 보낸다.
    if (FMath::Abs(DesiredYaw - CurrentYaw) >= P1MoveSync::YAW_TOLERANCE)
        bForceSend = true;

    // 공격 중에는 바로 보내지 않는다. 주기 송신은 막지 않는다.
    if (bAttacking)
        bForceSend = false;

    FP1MoveSendDecision Decision;
    Decision.NextTimer = RemainingTimer - DeltaSeconds;

    if (Decision.NextTimer <= 0.f || bForceSend)
    {
        Decision.bSend = true;
        Decision.NextTimer = P1MoveSync::MOVE_PACKET_SEND_DELAY;
    }

    return Decision;
}
