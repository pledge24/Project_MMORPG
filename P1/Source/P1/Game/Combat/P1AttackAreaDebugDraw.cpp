#include "Game/Combat/P1AttackAreaDebugDraw.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Game/Combat/P1AttackArea.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/Generated/P1ActionRow.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Utils/LogCategory.h"

namespace
{
    TAutoConsoleVariable<bool> CVarShowAttackAreas(
        TEXT("p1.Debug.ShowAttackAreas"),
        false,
        TEXT("켜면 액션이 재생될 때 타격 시점마다 그 타격의 공격 범위를 그린다."),
        ECVF_Cheat);

    TAutoConsoleVariable<float> CVarAttackAreaSeconds(
        TEXT("p1.Debug.AttackAreaSeconds"),
        3.0f,
        TEXT("공격 범위를 화면에 남기는 시간(초)."),
        ECVF_Cheat);

    /** 부채꼴의 호와 원을 나누는 구간 수. 반지름 250cm의 반원에서 선분 하나가 25cm 남짓이다. */
    constexpr int32 ARC_SEGMENTS = 32;

    /** 타격 순번마다 색을 바꿔 한 액션의 타격 여럿이 겹쳐도 구별되게 한다. */
    const FColor HIT_COLORS[] = { FColor::Red, FColor::Orange, FColor::Yellow, FColor::Cyan, FColor::Magenta };

    /** 높이 범위의 아래와 위에 윤곽선을 그리고 꼭짓점마다 세로선으로 잇는다. 공격 범위는 이 기둥 안이다. */
    void DrawHit(const UWorld* World, const FP1ActionHitRow& Hit, const FVector& AttackerFoot, float AttackerYaw)
    {
        const TArray<FVector2D> Outline = P1AttackArea::MakeOutline(Hit, ARC_SEGMENTS);
        const FColor Color = HIT_COLORS[FMath::Abs(Hit.HitIndex) % UE_ARRAY_COUNT(HIT_COLORS)];
        const float Seconds = CVarAttackAreaSeconds.GetValueOnGameThread();

        const auto DrawRing = [&](float Height, float Thickness)
        {
            for (int32 Index = 0; Index < Outline.Num(); ++Index)
            {
                DrawDebugLine(World,
                    P1AttackArea::ToWorld(Outline[Index], AttackerFoot, AttackerYaw, Height),
                    P1AttackArea::ToWorld(Outline[(Index + 1) % Outline.Num()], AttackerFoot, AttackerYaw, Height),
                    Color, false, Seconds, 0, Thickness);
            }
        };

        DrawRing(Hit.ZMin, 0.f);
        DrawRing(Hit.ZMax, 0.f);
        for (const FVector2D& Corner : Outline)
        {
            DrawDebugLine(World,
                P1AttackArea::ToWorld(Corner, AttackerFoot, AttackerYaw, Hit.ZMin),
                P1AttackArea::ToWorld(Corner, AttackerFoot, AttackerYaw, Hit.ZMax),
                Color, false, Seconds);
        }

        // 공격자의 발밑 높이에도 윤곽선을 그린다. 높이 범위가 바닥을 감싸면 바닥에 닿은 범위가 보인다.
        if (Hit.ZMin < 0.f && Hit.ZMax > 0.f)
            DrawRing(0.f, 2.f);
    }

    /** 콘솔 명령: 내 플레이어의 발밑과 방향 기준으로 액션의 공격 범위를 모두 그린다. */
    void DrawAttackAreaCommand(const TArray<FString>& Args, UWorld* World)
    {
        int32 ActionId = 0;
        if (Args.Num() < 1 || LexTryParseString(ActionId, *Args[0]) == false)
        {
            UE_LOG(LogP1Combat, Warning, TEXT("[공격 범위] 사용법: p1.Debug.DrawAttackArea <액션 번호>"));
            return;
        }

        const APlayerController* Controller = World != nullptr ? World->GetFirstPlayerController() : nullptr;
        const ACharacter* Player = Controller != nullptr ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
        if (Player == nullptr)
        {
            UE_LOG(LogP1Combat, Warning, TEXT("[공격 범위] 기준이 될 내 플레이어가 없다"));
            return;
        }

        // 서버처럼 범위의 높이를 발밑 기준으로 둔다. 액터 위치는 캡슐 중심이다.
        const FVector Foot = Player->GetActorLocation() - FVector(0.0, 0.0, Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
        const int32 HitCount = P1AttackAreaDebugDraw::DrawAction(World, ActionId, Foot, Player->GetActorRotation().Yaw);
        if (HitCount == 0)
            UE_LOG(LogP1Combat, Warning, TEXT("[공격 범위] 액션 %d이 액션 테이블에 없거나 타격이 없다"), ActionId);
    }

    FAutoConsoleCommandWithWorldAndArgs DrawAttackAreaConsoleCommand(
        TEXT("p1.Debug.DrawAttackArea"),
        TEXT("내 플레이어의 위치와 방향 기준으로 액션의 공격 범위를 타격마다 그린다. 인자: <액션 번호>"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DrawAttackAreaCommand),
        ECVF_Cheat);
}

void P1AttackAreaDebugDraw::DrawHitIfEnabled(const UWorld* World, int32 ActionId, int32 HitIndex, const FVector& AttackerFoot, float AttackerYaw)
{
    if (CVarShowAttackAreas.GetValueOnGameThread() == false)
        return;

    const FP1ActionRow* Action = UP1GameDataSettings::FindActionRow(ActionId);
    if (Action == nullptr || Action->Hits.IsValidIndex(HitIndex) == false)
    {
        UE_LOG(LogP1Combat, Warning, TEXT("[공격 범위] 액션 %d에 타격 %d이 없다"), ActionId, HitIndex);
        return;
    }

    DrawHit(World, Action->Hits[HitIndex], AttackerFoot, AttackerYaw);
}

int32 P1AttackAreaDebugDraw::DrawAction(const UWorld* World, int32 ActionId, const FVector& AttackerFoot, float AttackerYaw)
{
    const FP1ActionRow* Action = UP1GameDataSettings::FindActionRow(ActionId);
    if (Action == nullptr)
        return 0;

    for (const FP1ActionHitRow& Hit : Action->Hits)
        DrawHit(World, Hit, AttackerFoot, AttackerYaw);

    return Action->Hits.Num();
}
