#include "Sync/P1MoveSendThrottle.h"
#include "Sync/P1MoveSyncConstants.h"

FP1MoveSendThrottle FP1MoveSendThrottle::Decide(const FInput& In)
{
    bool bForceSend = In.bInputChanged && In.bCanInputMovement;

    // 각도를 감싸지 않는다. 179도와 -179도도 358도 차이로 보고 바로 보낸다.
    if (FMath::Abs(In.DesiredYaw - In.CurrentYaw) >= P1MoveSync::YAW_TOLERANCE)
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
