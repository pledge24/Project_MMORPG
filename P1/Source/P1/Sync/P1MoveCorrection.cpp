#include "Sync/P1MoveCorrection.h"
#include "Sync/P1MoveSyncConstants.h"
#include "Kismet/KismetMathLibrary.h"

using namespace P1MoveSync;

FP1MoveCorrection FP1MoveCorrection::Compute(
    const FVector& ClientLocation,
    const FRotator& ClientRotation,
    const FVector& ServerLocation,
    float ServerYaw,
    const FVector& MoveDirection,
    bool bCorrectRotation,
    float DeltaSeconds)
{
    FP1MoveCorrection Result;
    Result.Location = ClientLocation;
    Result.Rotation = ClientRotation;

    // 지금은 높이를 판정하지 않으므로 보정은 XY에만 적용한다. 서버 Z는 높이를 다룰 때를 위해 받아 둔다
    // (docs/backlog.md 「높이(Z) 판정 도입」). 그때 이 줄부터 고친다.
    const FVector Target(ServerLocation.X, ServerLocation.Y, ClientLocation.Z);

    if (FVector::Distance(ClientLocation, Target) >= SNAP_DISTANCE)
    {
        Result.Location = Target;
        Result.Rotation = FRotator(0.f, ServerYaw, 0.f);
        Result.bSnapped = true;
        return Result;
    }

    // float를 !=로 비교한다. 같은 값이 들어가도 RInterpTo가 현재 회전을 그대로 돌려주므로 무해하다.
    if (bCorrectRotation && ServerYaw != ClientRotation.Yaw)
    {
        const FRotator TargetRotation(0.f, ServerYaw, 0.f);
        Result.Rotation = FMath::RInterpTo(ClientRotation, TargetRotation, DeltaSeconds, ROTATION_INTERP_SPEED);
    }

    const FVector CorrectionPoint = MoveDirection == FVector::ZeroVector
        ? Target
        : UKismetMathLibrary::FindClosestPointOnLine(ClientLocation, Target, MoveDirection);

    Result.Location = FMath::VInterpTo(ClientLocation, CorrectionPoint, DeltaSeconds, LOCATION_INTERP_SPEED);

    return Result;
}
