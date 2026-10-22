// 접속 흐름 서브시스템이 허용된 단계 전이만 받는 규칙을 고정한다.
//
// 왜 이것인가: 로그인부터 게임까지의 순서가 게임 인스턴스, 인게임 컨트롤러, 로그인 관리자에 흩어져 있어서 지금 어느
// 단계인지 아무도 몰랐다. 이제 단계는 접속 흐름 서브시스템 하나가 갖고, 순서에 맞지 않는 전이를 거절한다.
// 레벨 전환은 전이를 일으킨 알림 처리가 따로 하므로, 이 테스트는 UWorld 없이 빈 게임 인스턴스 위에서 돈다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Core.EntryFlow

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Core/P1EntryFlowSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace P1EntryFlowTest
{
    UP1EntryFlowSubsystem* MakeFlow()
    {
        // 게임 인스턴스 서브시스템은 게임 인스턴스 안에서만 만들 수 있다. 초기화하지 않은 빈 인스턴스면 된다.
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        return NewObject<UP1EntryFlowSubsystem>(GameInstance);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1EntryFlowTest,
    "P1.Core.EntryFlow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1EntryFlowTest::RunTest(const FString& Parameters)
{
    using namespace P1EntryFlowTest;
    using EPhase = EP1EntryPhase;

    // 3)의 거절 여덟 번은 경고를 남긴다.
    AddExpectedMessage(TEXT("허용되지 않은 접속 단계 전이를 거절함"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 8, false);

    // 1) 로그인에서 시작해 로비, 맵 로드, 게임 순서로 나아가고, 맵이 바뀌면 게임에서 맵 로드로 돌아간다.
    {
        UP1EntryFlowSubsystem* Flow = MakeFlow();
        TestTrue(TEXT("처음은 로그인"), Flow->GetPhase() == EPhase::Login);

        TestTrue(TEXT("로그인 → 로비"), Flow->TryEnterPhase(EPhase::Lobby));
        TestTrue(TEXT("로비 → 맵 로드"), Flow->TryEnterPhase(EPhase::MapLoad));
        TestTrue(TEXT("맵 로드 → 게임"), Flow->TryEnterPhase(EPhase::Game));
        TestTrue(TEXT("게임 → 맵 로드"), Flow->TryEnterPhase(EPhase::MapLoad));
        TestTrue(TEXT("마지막 단계는 맵 로드"), Flow->GetPhase() == EPhase::MapLoad);
    }

    // 2) 연결이 끊기면 어느 단계에서든 로그인으로 돌아간다. 이미 로그인이면 전이가 아니다.
    {
        for (EPhase From : { EPhase::Lobby, EPhase::MapLoad, EPhase::Game })
            TestTrue(*FString::Printf(TEXT("%d → 로그인"), static_cast<int32>(From)), UP1EntryFlowSubsystem::CanTransition(From, EPhase::Login));

        TestFalse(TEXT("로그인 → 로그인"), UP1EntryFlowSubsystem::CanTransition(EPhase::Login, EPhase::Login));
    }

    // 3) 순서를 건너뛰거나 거꾸로 가는 전이는 거절하고 단계를 그대로 둔다.
    {
        UP1EntryFlowSubsystem* Flow = MakeFlow();
        TestFalse(TEXT("로그인 → 맵 로드"), Flow->TryEnterPhase(EPhase::MapLoad));
        TestFalse(TEXT("로그인 → 게임"), Flow->TryEnterPhase(EPhase::Game));
        TestTrue(TEXT("거절 뒤에도 로그인"), Flow->GetPhase() == EPhase::Login);

        Flow->TryEnterPhase(EPhase::Lobby);
        TestFalse(TEXT("로비 → 로비"), Flow->TryEnterPhase(EPhase::Lobby));
        TestFalse(TEXT("로비 → 게임"), Flow->TryEnterPhase(EPhase::Game));

        Flow->TryEnterPhase(EPhase::MapLoad);
        TestFalse(TEXT("맵 로드 → 로비"), Flow->TryEnterPhase(EPhase::Lobby));
        TestFalse(TEXT("맵 로드 → 맵 로드"), Flow->TryEnterPhase(EPhase::MapLoad));

        Flow->TryEnterPhase(EPhase::Game);
        TestFalse(TEXT("게임 → 로비"), Flow->TryEnterPhase(EPhase::Lobby));
        TestFalse(TEXT("게임 → 게임"), Flow->TryEnterPhase(EPhase::Game));
        TestTrue(TEXT("거절 뒤에도 게임"), Flow->GetPhase() == EPhase::Game);
    }

    return true;
}

#endif
