#include "Sync/P1MoveCorrection.h"
#include "Sync/P1MoveSyncConstants.h"
#include "Kismet/KismetMathLibrary.h"

FP1MoveCorrection FP1MoveCorrection::Compute(
    const FVector& ClientLocation,
    const FRotator& ClientRotation,
    const FVector& ServerLocation,
    float ServerYaw,
    const FVector& MoveDirection,
    bool bCorrectRotation,
    bool bInAction,
    float DeltaSeconds)
{
    FP1MoveCorrection Result;
    Result.Location = ClientLocation;
    Result.Rotation = ClientRotation;

    // 서버가 지형으로 높이를 판정하므로 높이도 서버 위치를 따른다. 경사를 오르는 원격 크리처가 평면 위치만 맞추면 땅에 묻히거나 뜬다.
    const FVector Target = ServerLocation;

    if (FVector::Distance(ClientLocation, Target) >= P1MoveSync::SNAP_DISTANCE)
    {
        Result.Location = Target;
        Result.Rotation = FRotator(0.f, ServerYaw, 0.f);
        Result.bSnapped = true;
        return Result;
    }

    // 루트 모션이 들어간 공격 중에는 몽타주가 위치와 방향을 정한다.
    if (bInAction)
        return Result;

    // float를 !=로 비교한다. 같은 값이 들어가도 RInterpTo가 현재 회전을 그대로 돌려주므로 무해하다.
    if (bCorrectRotation && ServerYaw != ClientRotation.Yaw)
    {
        const FRotator TargetRotation(0.f, ServerYaw, 0.f);
        Result.Rotation = FMath::RInterpTo(ClientRotation, TargetRotation, DeltaSeconds, P1MoveSync::ROTATION_INTERP_SPEED);
    }

    // 정지 중이면 서버 위치를 향한다. 이동 중이면 서버 위치를 지나는 이동 방향 직선 위에서
    // 현재 위치와 가장 가까운 점을 향한다.
    const FVector CorrectionPoint = MoveDirection == FVector::ZeroVector
        ? Target
        : UKismetMathLibrary::FindClosestPointOnLine(ClientLocation, Target, MoveDirection);

    Result.Location = FMath::VInterpTo(ClientLocation, CorrectionPoint, DeltaSeconds, P1MoveSync::LOCATION_INTERP_SPEED);

    return Result;
}
