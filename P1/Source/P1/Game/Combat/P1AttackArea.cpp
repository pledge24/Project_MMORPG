#include "Game/Combat/P1AttackArea.h"
#include "Game/Data/Generated/P1ActionHitRow.h"

// 서버의 공격 범위 판정과 같은 규칙이다. 한쪽을 고치면 다른 쪽과, 서버 테스트와 클라이언트 테스트가 함께 읽는 공용 사례
// 파일도 고쳐야 화면의 범위와 서버의 판정이 어긋나지 않는다.
// 경계 사례의 답이 서버와 같도록 서버처럼 float로 계산한다.
namespace
{
    /** 형상 기준점에서 본 점(앞 +X, 오른쪽 +Y)과 형상 사이의 평면 거리. 형상 안이면 0이다. */
    float DistanceToShape(const FP1ActionHitRow& Hit, float X, float Y)
    {
        switch (Hit.Shape)
        {
        case EP1AttackAreaShape::CIRCLE:
            return FMath::Max(0.f, FMath::Sqrt(X * X + Y * Y) - Hit.Radius);

        case EP1AttackAreaShape::SECTOR:
        {
            const float HalfAngle = Hit.Angle * 0.5f;
            const float Distance = FMath::Sqrt(X * X + Y * Y);
            if (HalfAngle >= 180.f || Distance == 0.f)
                return FMath::Max(0.f, Distance - Hit.Radius);

            // 좌우가 대칭이므로 점을 오른쪽으로 접고, 오른쪽 가장자리 선분 하나와만 잰다.
            const float FoldedY = FMath::Abs(Y);
            if (FMath::Atan2(FoldedY, X) <= FMath::DegreesToRadians(HalfAngle))
                return FMath::Max(0.f, Distance - Hit.Radius);

            // 각도 밖이면 가장 가까운 곳은 가장자리 선분(꼭짓점과 호의 끝 포함) 위에 있다.
            const float EdgeX = FMath::Cos(FMath::DegreesToRadians(HalfAngle));
            const float EdgeY = FMath::Sin(FMath::DegreesToRadians(HalfAngle));
            const float Along = FMath::Clamp(X * EdgeX + FoldedY * EdgeY, 0.f, Hit.Radius);
            const float GapX = X - EdgeX * Along;
            const float GapY = FoldedY - EdgeY * Along;
            return FMath::Sqrt(GapX * GapX + GapY * GapY);
        }

        case EP1AttackAreaShape::RECTANGLE:
        {
            // 직사각형은 기준점에서 앞으로 뻗고, 폭은 기준점의 좌우로 절반씩이다.
            const float OutsideX = FMath::Max3(-X, 0.f, X - Hit.Length);
            const float OutsideY = FMath::Max(0.f, FMath::Abs(Y) - Hit.Width * 0.5f);
            return FMath::Sqrt(OutsideX * OutsideX + OutsideY * OutsideY);
        }

        default:
            return TNumericLimits<float>::Max();
        }
    }

    /** Center를 중심으로 FromDegrees에서 ToDegrees까지의 호를 Segments 구간으로 나눈 점. 양 끝을 모두 담는다. */
    void AppendArc(TArray<FVector2D>& Out, const FVector2D& Center, float Radius, float FromDegrees, float ToDegrees, int32 Segments)
    {
        for (int32 Index = 0; Index <= Segments; ++Index)
        {
            const float Radians = FMath::DegreesToRadians(FMath::Lerp(FromDegrees, ToDegrees, static_cast<float>(Index) / Segments));
            Out.Add(Center + FVector2D(FMath::Cos(Radians), FMath::Sin(Radians)) * Radius);
        }
    }
}

bool P1AttackArea::Overlaps(const FP1ActionHitRow& Hit, const FVector& AttackerFoot, float AttackerYaw,
    const FVector& TargetFoot, float TargetRadius, float TargetHalfHeight)
{
    // 높이 범위는 공격자 발밑 기준이고, 대상은 발밑에서 반높이의 두 배까지 선다.
    const float AreaBottom = static_cast<float>(AttackerFoot.Z) + Hit.ZMin;
    const float AreaTop = static_cast<float>(AttackerFoot.Z) + Hit.ZMax;
    const float TargetBottom = static_cast<float>(TargetFoot.Z);
    const float TargetTop = TargetBottom + TargetHalfHeight * 2.f;
    if (TargetTop < AreaBottom - OverlapTolerance || TargetBottom > AreaTop + OverlapTolerance)
        return false;

    // 대상 위치를 공격자 기준으로 돌린 뒤 형상 기준점으로 옮긴다. ToWorld의 역변환이다.
    const float YawRadians = FMath::DegreesToRadians(AttackerYaw);
    const float ForwardX = FMath::Cos(YawRadians);
    const float ForwardY = FMath::Sin(YawRadians);
    // 서버는 float 좌표끼리 뺀다. 먼저 빼고 바꾸면 원점에서 먼 경계 사례의 답이 서버와 갈릴 수 있다.
    const float Dx = static_cast<float>(TargetFoot.X) - static_cast<float>(AttackerFoot.X);
    const float Dy = static_cast<float>(TargetFoot.Y) - static_cast<float>(AttackerFoot.Y);
    const float LocalX = Dx * ForwardX + Dy * ForwardY - Hit.OffsetX;
    const float LocalY = -Dx * ForwardY + Dy * ForwardX - Hit.OffsetY;

    return DistanceToShape(Hit, LocalX, LocalY) <= TargetRadius + OverlapTolerance;
}

TArray<FVector2D> P1AttackArea::MakeOutline(const FP1ActionHitRow& Hit, int32 ArcSegments)
{
    const FVector2D Origin(Hit.OffsetX, Hit.OffsetY);
    const int32 Segments = FMath::Max(ArcSegments, 3);
    TArray<FVector2D> Outline;

    switch (Hit.Shape)
    {
    case EP1AttackAreaShape::SECTOR:
        // 360도 미만이면 앞 방향을 가운데에 두고 좌우로 절반씩 벌어진다. 360도 이상은 원으로 그린다.
        // 꼭짓점을 넣으면 원 안을 가로지르는 선이 생긴다.
        if (Hit.Angle < 360.f)
        {
            Outline.Add(Origin);
            AppendArc(Outline, Origin, Hit.Radius, -Hit.Angle * 0.5f, Hit.Angle * 0.5f, Segments);
            break;
        }
        [[fallthrough]];

    case EP1AttackAreaShape::CIRCLE:
        // 호의 끝은 시작과 같은 점이라 뺀다.
        AppendArc(Outline, Origin, Hit.Radius, 0.f, 360.f, Segments);
        Outline.Pop();
        break;

    case EP1AttackAreaShape::RECTANGLE:
    {
        const float HalfWidth = Hit.Width * 0.5f;
        Outline.Add(Origin + FVector2D(0.0, -HalfWidth));
        Outline.Add(Origin + FVector2D(Hit.Length, -HalfWidth));
        Outline.Add(Origin + FVector2D(Hit.Length, HalfWidth));
        Outline.Add(Origin + FVector2D(0.0, HalfWidth));
        break;
    }

    default:
        break;
    }

    return Outline;
}

FVector P1AttackArea::ToWorld(const FVector2D& Local, const FVector& AttackerFoot, float AttackerYaw, float Height)
{
    // 방향은 +X에서 +Y 쪽으로 돈다. 언리얼의 yaw와 같다.
    const double YawRadians = FMath::DegreesToRadians(static_cast<double>(AttackerYaw));
    const double Cos = FMath::Cos(YawRadians);
    const double Sin = FMath::Sin(YawRadians);
    return FVector(
        AttackerFoot.X + Local.X * Cos - Local.Y * Sin,
        AttackerFoot.Y + Local.X * Sin + Local.Y * Cos,
        AttackerFoot.Z + Height);
}
