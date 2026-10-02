#include "Sync/P1MoveCorrection.h"
#include "Kismet/KismetMathLibrary.h"

namespace
{
    /** 이 거리(cm) 이상 벌어지면 보간하지 않고 서버 위치로 옮긴다. */
    constexpr float SNAP_DISTANCE = 800.f;

    /** FMath::VInterpTo에 넘기는 보간 속도다. */
    constexpr float LOCATION_INTERP_SPEED = 5.f;

    /** FMath::RInterpTo에 넘기는 보간 속도다. */
    constexpr float ROTATION_INTERP_SPEED = 5.f;
}

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
