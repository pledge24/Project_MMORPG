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
    // 적지 않은 입력 필드는 FInput의 기본값(false, 0)이다.

    // 1) 주기가 남았고 즉시 보낼 이유가 없으면 보내지 않고 타이머만 줄인다.
    {
        const FP1MoveSendThrottle Decision = FP1MoveSendThrottle::Decide({.RemainingTimer = 0.2f, .DeltaSeconds = 0.05f, .bCanInputMovement = true});
        TestFalse(TEXT("주기가 남았으면 보내지 않는다"), Decision.bSend);
        TestEqual(TEXT("타이머가 프레임 시간만큼 준다"), Decision.NextTimer, 0.15f, KINDA_SMALL_NUMBER);
    }

    // 2) 주기가 끝나면 보내고 타이머를 전송 주기로 되돌린다. 0에 딱 닿아도 보낸다.
    {
        const FP1MoveSendThrottle Decision = FP1MoveSendThrottle::Decide({.RemainingTimer = 0.05f, .DeltaSeconds = 0.1f, .bCanInputMovement = true});
        TestTrue(TEXT("주기가 끝나면 보낸다"), Decision.bSend);
        TestEqual(TEXT("보낸 뒤 타이머는 0.2초다"), Decision.NextTimer, 0.2f, KINDA_SMALL_NUMBER);

        const FP1MoveSendThrottle Exact = FP1MoveSendThrottle::Decide({.RemainingTimer = 0.1f, .DeltaSeconds = 0.1f, .bCanInputMovement = true});
        TestTrue(TEXT("타이머가 0에 닿으면 보낸다"), Exact.bSend);
    }

    // 3) 입력이 바뀌면 주기를 기다리지 않고 보낸다. 이동 입력이 막혀 있으면 입력 변화로는 보내지 않는다.
    {
        const FP1MoveSendThrottle Changed = FP1MoveSendThrottle::Decide({.RemainingTimer = 0.2f, .DeltaSeconds = 0.01f, .bInputChanged = true, .bCanInputMovement = true});
        TestTrue(TEXT("입력이 바뀌면 바로 보낸다"), Changed.bSend);
        TestEqual(TEXT("바로 보내도 타이머는 0.2초로 돌아간다"), Changed.NextTimer, 0.2f, KINDA_SMALL_NUMBER);

        const FP1MoveSendThrottle Blocked = FP1MoveSendThrottle::Decide({.RemainingTimer = 0.2f, .DeltaSeconds = 0.01f, .bInputChanged = true});
        TestFalse(TEXT("이동 입력이 막혀 있으면 입력 변화로 보내지 않는다"), Blocked.bSend);
    }

    // 4) 입력이 있으면 원하는 이동 Yaw와 현재 Yaw가 60도 이상 벌어질 때 바로 보낸다. 경계값 60도 보내는 쪽이다.
    //    이 판정은 이동 입력이 막혀 있어도 한다. 차이는 ±180도에서 감싸서 짧은 쪽으로 잰다(#156).
    {
        TestTrue(TEXT("60도 차이면 바로 보낸다"),
            FP1MoveSendThrottle::Decide({.RemainingTimer = 0.2f, .DeltaSeconds = 0.01f, .bCanInputMovement = true,
                .bHasMoveInput = true, .DesiredYaw = 70.f, .CurrentYaw = 10.f}).bSend);
        TestFalse(TEXT("60도 미만이면 보내지 않는다"),
            FP1MoveSendThrottle::Decide({.RemainingTimer = 0.2f, .DeltaSeconds = 0.01f, .bCanInputMovement = true,
                .bHasMoveInput = true, .DesiredYaw = 69.f, .CurrentYaw = 10.f}).bSend);
        TestTrue(TEXT("이동 입력이 막혀 있어도 회전으로는 보낸다"),
            FP1MoveSendThrottle::Decide({.RemainingTimer = 0.2f, .DeltaSeconds = 0.01f,
                .bHasMoveInput = true, .DesiredYaw = 70.f, .CurrentYaw = 10.f}).bSend);
        TestFalse(TEXT("179도와 -179도는 2도 차이라 보내지 않는다"),
            FP1MoveSendThrottle::Decide({.RemainingTimer = 0.2f, .DeltaSeconds = 0.01f, .bCanInputMovement = true,
                .bHasMoveInput = true, .DesiredYaw = 179.f, .CurrentYaw = -179.f}).bSend);
        TestTrue(TEXT("150도와 -150도는 감싸서 60도 차이라 보낸다"),
            FP1MoveSendThrottle::Decide({.RemainingTimer = 0.2f, .DeltaSeconds = 0.01f, .bCanInputMovement = true,
                .bHasMoveInput = true, .DesiredYaw = 150.f, .CurrentYaw = -150.f}).bSend);
    }

    // 5) 공격 중에는 입력 변화와 회전으로 바로 보내지 않는다. 주기가 끝나면 공격 중이어도 보낸다.
    {
        const FP1MoveSendThrottle Suppressed = FP1MoveSendThrottle::Decide({.RemainingTimer = 0.2f, .DeltaSeconds = 0.01f,
            .bInputChanged = true, .bCanInputMovement = true, .bHasMoveInput = true, .DesiredYaw = 70.f, .CurrentYaw = 10.f,
            .bAttacking = true});
        TestFalse(TEXT("공격 중에는 바로 보내지 않는다"), Suppressed.bSend);
        TestEqual(TEXT("보내지 않았으면 타이머만 준다"), Suppressed.NextTimer, 0.19f, KINDA_SMALL_NUMBER);

        TestTrue(TEXT("공격 중이어도 주기가 끝나면 보낸다"),
            FP1MoveSendThrottle::Decide({.RemainingTimer = 0.05f, .DeltaSeconds = 0.1f, .bCanInputMovement = true, .bAttacking = true}).bSend);
    }

    // 6) 입력이 없으면 원하는 이동 Yaw를 보지 않는다. 입력을 떼면 그 값이 0이 되기 때문이다(#156).
    //    대신 서버가 마지막으로 받은 yaw와 현재 yaw가 60도 이상 벌어지면 바로 보낸다. 공격 중에는 보내지 않는다.
    {
        TestFalse(TEXT("입력을 뗀 뒤 원하는 이동 Yaw가 0이어도 회전으로 보내지 않는다"),
            FP1MoveSendThrottle::Decide({.RemainingTimer = 0.2f, .DeltaSeconds = 0.01f, .bCanInputMovement = true,
                .DesiredYaw = 0.f, .CurrentYaw = 90.f, .LastSentYaw = 90.f}).bSend);
        TestTrue(TEXT("입력이 없어도 마지막으로 보낸 yaw에서 60도 이상 돌면 보낸다"),
            FP1MoveSendThrottle::Decide({.RemainingTimer = 0.2f, .DeltaSeconds = 0.01f, .bCanInputMovement = true,
                .CurrentYaw = 90.f, .LastSentYaw = 30.f}).bSend);
        TestFalse(TEXT("마지막으로 보낸 yaw와의 차이도 감싸서 잰다"),
            FP1MoveSendThrottle::Decide({.RemainingTimer = 0.2f, .DeltaSeconds = 0.01f, .bCanInputMovement = true,
                .CurrentYaw = -179.f, .LastSentYaw = 179.f}).bSend);
        TestFalse(TEXT("공격 중에는 마지막으로 보낸 yaw에서 돌아도 바로 보내지 않는다"),
            FP1MoveSendThrottle::Decide({.RemainingTimer = 0.2f, .DeltaSeconds = 0.01f, .bCanInputMovement = true,
                .CurrentYaw = 90.f, .LastSentYaw = 30.f, .bAttacking = true}).bSend);
    }

    return true;
}

#endif
