// 화면 중앙 조준으로 NPC에 초점을 맞추는 판정을 고정한다.
//
// 왜 이것인가: 상점을 볼륨에 들어가면 열던 때는 상점 근처를 지나가기만 해도 창이 떴다. 지금은 비전투 모드에서
// 사거리 안의 NPC를 조준해야 「상점 열기」가 뜬다. 판정이 틀리면 멀리서 상점이 열리거나(서버가 거절한다), 전투 중에
// 상호작용 표시가 공격 클릭을 가로채거나, NPC 둘이 겹쳐 보일 때 뒤의 NPC가 열린다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Interaction.NpcFocus

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Game/Interaction/P1NpcFocus.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace P1NpcFocusTest
{
    constexpr float RANGE = 300.f;
    const FVector PLAYER(0.0, 0.0, 90.0);

    /** 스윕이 Distance만큼 가서 맞힌 것. Location은 맞힌 액터의 위치다. */
    FP1AimHit MakeHit(float Distance, double X, double Y, bool bNpc = true)
    {
        FP1AimHit Hit;
        Hit.Distance = Distance;
        Hit.ActorLocation = FVector(X, Y, 90.0);
        Hit.bNpc = bNpc;
        return Hit;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1NpcFocusTest,
    "P1.Interaction.NpcFocus",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1NpcFocusTest::RunTest(const FString& Parameters)
{
    using namespace P1NpcFocusTest;

    // 1) 사거리 안의 NPC를 조준하면 초점이 맞는다. 거리는 플레이어와 NPC 사이의 평면 거리다.
    {
        const TArray<FP1AimHit> Hits = { MakeHit(600.f, 250.0, 0.0) };
        TestEqual(TEXT("사거리 안"), P1NpcFocus::PickFocus(Hits, PLAYER, RANGE, false), 0);

        // 높이 차이는 보지 않는다. 계단 위의 NPC도 평면 거리로 잰다.
        FP1AimHit Above = MakeHit(600.f, 250.0, 0.0);
        Above.ActorLocation.Z = 500.0;
        TestEqual(TEXT("높이가 다른 사거리 안"), P1NpcFocus::PickFocus({ Above }, PLAYER, RANGE, false), 0);
    }

    // 2) 사거리 밖의 NPC는 조준해도 초점이 맞지 않는다. 스윕 거리가 짧아도 플레이어에게서 멀면 밖이다.
    {
        const TArray<FP1AimHit> Hits = { MakeHit(200.f, 301.0, 0.0) };
        TestEqual(TEXT("사거리 밖"), P1NpcFocus::PickFocus(Hits, PLAYER, RANGE, false), INDEX_NONE);
        TestEqual(TEXT("사거리 경계"), P1NpcFocus::PickFocus({ MakeHit(200.f, 0.0, 300.0) }, PLAYER, RANGE, false), 0);
    }

    // 3) 전투 모드에서는 아무 반응도 없다. 왼쪽 클릭이 공격이다.
    {
        const TArray<FP1AimHit> Hits = { MakeHit(600.f, 250.0, 0.0) };
        TestEqual(TEXT("전투 모드"), P1NpcFocus::PickFocus(Hits, PLAYER, RANGE, true), INDEX_NONE);
    }

    // 4) 조준선에 NPC 둘이 걸리면 앞의 것이다. 스윕은 맞힌 순서를 보장하지 않으므로 거리로 고른다.
    {
        const TArray<FP1AimHit> Hits = { MakeHit(650.f, 280.0, 0.0), MakeHit(550.f, 150.0, 0.0) };
        TestEqual(TEXT("앞의 NPC"), P1NpcFocus::PickFocus(Hits, PLAYER, RANGE, false), 1);

        // 앞의 NPC가 사거리 밖이면 뒤의 NPC로 넘어가지 않는다. 플레이어가 보는 것은 앞의 NPC다.
        const TArray<FP1AimHit> FarFront = { MakeHit(700.f, 100.0, 0.0), MakeHit(500.f, 400.0, 0.0) };
        TestEqual(TEXT("사거리 밖의 앞 NPC"), P1NpcFocus::PickFocus(FarFront, PLAYER, RANGE, false), INDEX_NONE);
    }

    // 5) NPC 앞을 가린 벽이나 몬스터가 먼저 맞으면 초점이 맞지 않는다.
    {
        const TArray<FP1AimHit> Hits = { MakeHit(600.f, 250.0, 0.0), MakeHit(400.f, 100.0, 0.0, false) };
        TestEqual(TEXT("가려진 NPC"), P1NpcFocus::PickFocus(Hits, PLAYER, RANGE, false), INDEX_NONE);
        TestEqual(TEXT("맞힌 것이 없음"), P1NpcFocus::PickFocus({}, PLAYER, RANGE, false), INDEX_NONE);
    }

    return true;
}

#endif
