// 원격 크리처를 서버 위치로 끌어당기는 보정 계산을 고정한다.
//
// 왜 이것인가: 다른 플레이어와 몬스터가 화면에서 어떻게 움직이는지가 전부 이 계산에 달렸다.
// 이동 동기화를 Sync 컴포넌트로 옮기는 작업(#128)이 이 계산을 다시 배치하므로, 옮기기 전에
// 지금 동작을 묶어 둔다.
//
// 기대값은 엔진 보간 함수의 수치를 다시 계산하지 않고 성질로 확인한다. 「목표에 가까워졌다」,
// 「한 틱에 도달하지 않았다」처럼 적어야 VInterpTo의 구현이 바뀌어도 테스트가 살아남는다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Sync/P1MoveCorrection.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1MoveCorrectionTest,
    "P1.Sync.MoveCorrection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1MoveCorrectionTest::RunTest(const FString& Parameters)
{
    const float DeltaSeconds = 1.f / 60.f;
    const FRotator ClientRotation(0.f, 10.f, 0.f);

    // 1) 서버 위치와 800 이상 벌어지면 보간하지 않고 서버 위치로 옮긴다. 경계값 800도 옮기는 쪽이다.
    //    회전 보정을 끈 크리처도 회전을 서버 yaw로 즉시 맞춘다.
    {
        const FVector Client(0.f, 0.f, 100.f);
        const FVector Server(800.f, 0.f, 999.f);

        const FP1MoveCorrection Result = FP1MoveCorrection::Compute(
            Client, ClientRotation, Server, 90.f, FVector::ZeroVector, false, DeltaSeconds);

        TestTrue(TEXT("경계값 800에서 순간이동한다"), Result.bSnapped);
        TestEqual(TEXT("서버의 XY로 옮긴다"), Result.Location, FVector(800.f, 0.f, 100.f));
        TestEqual(TEXT("회전을 서버 yaw로 즉시 맞춘다"), Result.Rotation, FRotator(0.f, 90.f, 0.f));
    }

    // 2) 800 미만이고 정지 중이면 서버 위치를 향해 다가가되 한 틱에 도달하지 않는다.
    {
        const FVector Client(0.f, 0.f, 100.f);
        const FVector Server(100.f, 0.f, 999.f);

        const FP1MoveCorrection Result = FP1MoveCorrection::Compute(
            Client, ClientRotation, Server, 10.f, FVector::ZeroVector, false, DeltaSeconds);

        TestFalse(TEXT("800 미만이면 순간이동하지 않는다"), Result.bSnapped);
        TestTrue(TEXT("서버 쪽으로 움직인다"), Result.Location.X > 0.f);
        TestTrue(TEXT("한 틱에 서버 위치까지 가지 않는다"), Result.Location.X < 100.f);
        TestEqual(TEXT("목표 직선을 벗어나지 않는다"), Result.Location.Y, 0.0);
    }

    // 3) 이동 중이면 서버 위치 자체가 아니라, 서버 위치를 지나 이동 방향과 평행한 직선 위에서
    //    현재 위치와 가장 가까운 점을 향한다. 진행 방향의 위치는 이동 컴포넌트에 맡기고 옆으로
    //    벗어난 만큼만 당긴다.
    {
        const FVector Client(0.f, 0.f, 100.f);
        const FVector Server(100.f, 50.f, 999.f);
        const FVector MoveDirection(1.f, 0.f, 0.f);

        const FP1MoveCorrection Result = FP1MoveCorrection::Compute(
            Client, ClientRotation, Server, 10.f, MoveDirection, false, DeltaSeconds);

        TestFalse(TEXT("800 미만이면 순간이동하지 않는다"), Result.bSnapped);
        TestEqual(TEXT("이동 방향 성분은 그대로다"), Result.Location.X, 0.0);
        TestTrue(TEXT("서버 직선 쪽으로 당겨진다"), Result.Location.Y > 0.f);
        TestTrue(TEXT("한 틱에 직선까지 가지 않는다"), Result.Location.Y < 50.f);
    }

    // 4) 지금은 높이를 판정하지 않으므로 서버가 보낸 Z를 쓰지 않는다. 순간이동하든 다가가든 Z는
    //    클라이언트의 Z 그대로다. 높이를 다루게 되면 이 단계의 기대값을 바꾼다.
    {
        const FVector Client(0.f, 0.f, 100.f);

        const FP1MoveCorrection Snapped = FP1MoveCorrection::Compute(
            Client, ClientRotation, FVector(900.f, 0.f, 999.f), 10.f, FVector::ZeroVector, false, DeltaSeconds);
        const FP1MoveCorrection Approached = FP1MoveCorrection::Compute(
            Client, ClientRotation, FVector(100.f, 50.f, 999.f), 10.f, FVector(1.f, 0.f, 0.f), false, DeltaSeconds);

        TestEqual(TEXT("순간이동해도 Z는 그대로다"), Snapped.Location.Z, 100.0);
        TestEqual(TEXT("다가가도 Z는 그대로다"), Approached.Location.Z, 100.0);
    }

    // 5) 회전 보정을 켜면 서버 yaw 쪽으로 돌되 한 틱에 다 돌지 않는다.
    //    꺼져 있으면 800 미만에서는 회전이 그대로다.
    {
        const FVector Client(0.f, 0.f, 100.f);
        const FVector Server(100.f, 0.f, 999.f);

        const FP1MoveCorrection On = FP1MoveCorrection::Compute(
            Client, ClientRotation, Server, 90.f, FVector::ZeroVector, true, DeltaSeconds);

        TestTrue(TEXT("켜면 서버 yaw 쪽으로 돈다"), On.Rotation.Yaw > ClientRotation.Yaw);
        TestTrue(TEXT("한 틱에 서버 yaw까지 돌지 않는다"), On.Rotation.Yaw < 90.f);

        const FP1MoveCorrection Off = FP1MoveCorrection::Compute(
            Client, ClientRotation, Server, 90.f, FVector::ZeroVector, false, DeltaSeconds);

        TestEqual(TEXT("끄면 회전이 그대로다"), Off.Rotation, ClientRotation);
    }

    return true;
}

#endif
