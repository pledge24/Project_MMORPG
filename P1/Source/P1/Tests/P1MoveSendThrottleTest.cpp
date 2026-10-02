// 내 플레이어의 이동 패킷을 언제 보낼지 정하는 판정을 고정한다.
//
// 왜 이것인가: 다른 클라이언트에 내 플레이어가 어떻게 보이는지가 이 판정에 달렸다. 이동 동기화를
// Sync 컴포넌트로 옮기는 작업(#128)이 AP1MyPlayer::Tick에 있던 이 판정을 떼어 냈으므로, 그때의 동작을
// 묶어 둔다.
//
// 기대값은 판정을 다시 계산하지 않고 원래 코드의 규칙에서 적었다. 전송 주기는 0.2초, 회전 허용치는 60도다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Sync/P1MoveSendThrottle.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1MoveSendThrottleTest,
    "P1.Sync.MoveSendThrottle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1MoveSendThrottleTest::RunTest(const FString& Parameters)
{
    // 1) 주기가 남았고 즉시 보낼 이유가 없으면 보내지 않고 타이머만 줄인다.
    {
        const FP1MoveSendDecision Decision = FP1MoveSendThrottle::Decide(0.2f, 0.05f, false, true, 0.f, 0.f, false);
        TestFalse(TEXT("주기가 남았으면 보내지 않는다"), Decision.bSend);
        TestEqual(TEXT("타이머가 프레임 시간만큼 준다"), Decision.NextTimer, 0.15f, KINDA_SMALL_NUMBER);
    }

    // 2) 주기가 끝나면 보내고 타이머를 전송 주기로 되돌린다. 0에 딱 닿아도 보낸다.
    {
        const FP1MoveSendDecision Decision = FP1MoveSendThrottle::Decide(0.05f, 0.1f, false, true, 0.f, 0.f, false);
        TestTrue(TEXT("주기가 끝나면 보낸다"), Decision.bSend);
        TestEqual(TEXT("보낸 뒤 타이머는 0.2초다"), Decision.NextTimer, 0.2f, KINDA_SMALL_NUMBER);

        const FP1MoveSendDecision Exact = FP1MoveSendThrottle::Decide(0.1f, 0.1f, false, true, 0.f, 0.f, false);
        TestTrue(TEXT("타이머가 0에 닿으면 보낸다"), Exact.bSend);
    }

    // 3) 입력이 바뀌면 주기를 기다리지 않고 보낸다. 이동 입력이 막혀 있으면 입력 변화로는 보내지 않는다.
    {
        const FP1MoveSendDecision Changed = FP1MoveSendThrottle::Decide(0.2f, 0.01f, true, true, 0.f, 0.f, false);
        TestTrue(TEXT("입력이 바뀌면 바로 보낸다"), Changed.bSend);
        TestEqual(TEXT("바로 보내도 타이머는 0.2초로 돌아간다"), Changed.NextTimer, 0.2f, KINDA_SMALL_NUMBER);

        const FP1MoveSendDecision Blocked = FP1MoveSendThrottle::Decide(0.2f, 0.01f, true, false, 0.f, 0.f, false);
        TestFalse(TEXT("이동 입력이 막혀 있으면 입력 변화로 보내지 않는다"), Blocked.bSend);
    }

    // 4) 원하는 이동 Yaw와 현재 Yaw가 60도 이상 벌어지면 바로 보낸다. 경계값 60도 보내는 쪽이다.
    //    이 판정은 이동 입력이 막혀 있어도 한다. 차이는 각도를 감싸지 않고 두 값의 차의 절댓값으로 잰다.
    {
        TestTrue(TEXT("60도 차이면 바로 보낸다"),
            FP1MoveSendThrottle::Decide(0.2f, 0.01f, false, true, 70.f, 10.f, false).bSend);
        TestFalse(TEXT("60도 미만이면 보내지 않는다"),
            FP1MoveSendThrottle::Decide(0.2f, 0.01f, false, true, 69.f, 10.f, false).bSend);
        TestTrue(TEXT("이동 입력이 막혀 있어도 회전으로는 보낸다"),
            FP1MoveSendThrottle::Decide(0.2f, 0.01f, false, false, 70.f, 10.f, false).bSend);
        TestTrue(TEXT("179도와 -179도는 358도 차이로 잰다"),
            FP1MoveSendThrottle::Decide(0.2f, 0.01f, false, true, 179.f, -179.f, false).bSend);
    }

    // 5) 공격 중에는 입력 변화와 회전으로 바로 보내지 않는다. 주기가 끝나면 공격 중이어도 보낸다.
    {
        const FP1MoveSendDecision Suppressed = FP1MoveSendThrottle::Decide(0.2f, 0.01f, true, true, 70.f, 10.f, true);
        TestFalse(TEXT("공격 중에는 바로 보내지 않는다"), Suppressed.bSend);
        TestEqual(TEXT("보내지 않았으면 타이머만 준다"), Suppressed.NextTimer, 0.19f, KINDA_SMALL_NUMBER);

        TestTrue(TEXT("공격 중이어도 주기가 끝나면 보낸다"),
            FP1MoveSendThrottle::Decide(0.05f, 0.1f, false, true, 0.f, 0.f, true).bSend);
    }

    return true;
}

#endif
