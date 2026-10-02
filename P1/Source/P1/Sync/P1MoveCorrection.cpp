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

    // 서버의 Z를 버리는 것은 AP1Creature에서 옮기기 전의 동작 그대로다. 의도인지는 확인하지 못했다
    // (docs/work/2026-10-02-client-structure.md 「기록」).
    const FVector Target(ServerLocation.X, ServerLocation.Y, ClientLocation.Z);

    if (FVector::Distance(ClientLocation, Target) >= SNAP_DISTANCE)
    {
        Result.Location = Target;
        Result.Rotation = FRotator(0.f, ServerYaw, 0.f);
        Result.bSnapped = true;
        return Result;
    }

    // float를 !=로 비교하는 것도 옮기기 전의 동작 그대로다. 결과가 같으면 RInterpTo가 그대로 돌려준다.
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
