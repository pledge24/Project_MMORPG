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

    // 3)의 거절 열다섯 번은 경고를 남긴다.
    AddExpectedMessage(TEXT("허용되지 않은 접속 단계 전이를 거절함"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 15, false);

    // 1) 로그인에서 시작해 월드 선택, 로비, 맵 로드, 게임 순서로 나아가고, 맵이 바뀌면 게임에서 맵 로드로 돌아간다.
    //    단계가 바뀔 때마다 바뀐 단계를 알린다.
    {
        UP1EntryFlowSubsystem* Flow = MakeFlow();
        TArray<EPhase> Notified;
        Flow->OnPhaseChanged.AddLambda([&Notified](EPhase Phase) { Notified.Add(Phase); });
        TestTrue(TEXT("처음은 로그인"), Flow->GetPhase() == EPhase::Login);

        TestTrue(TEXT("로그인 → 월드 선택"), Flow->TryEnterPhase(EPhase::WorldSelect));
        TestTrue(TEXT("월드 선택 → 로비"), Flow->TryEnterPhase(EPhase::Lobby));
        TestTrue(TEXT("로비 → 맵 로드"), Flow->TryEnterPhase(EPhase::MapLoad));
        TestTrue(TEXT("맵 로드 → 게임"), Flow->TryEnterPhase(EPhase::Game));
        TestTrue(TEXT("게임 → 맵 로드"), Flow->TryEnterPhase(EPhase::MapLoad));
        TestTrue(TEXT("마지막 단계는 맵 로드"), Flow->GetPhase() == EPhase::MapLoad);

        const TArray<EPhase> Expected = { EPhase::WorldSelect, EPhase::Lobby, EPhase::MapLoad, EPhase::Game, EPhase::MapLoad };
        TestTrue(TEXT("바뀐 단계를 차례로 알린다"), Notified == Expected);
    }

    // 2) 연결이 끊기거나 로그인 세션이 만료되면 어느 단계에서든 로그인으로 돌아간다. 이미 로그인이면 전이가 아니다.
    //    로비와 게임에서는 다시 로그인하지 않고 월드 선택으로 돌아갈 수 있다.
    {
        for (EPhase From : { EPhase::WorldSelect, EPhase::Lobby, EPhase::MapLoad, EPhase::Game, EPhase::ReturningToLobby })
            TestTrue(*FString::Printf(TEXT("%d → 로그인"), static_cast<int32>(From)), UP1EntryFlowSubsystem::CanTransition(From, EPhase::Login));

        TestFalse(TEXT("로그인 → 로그인"), UP1EntryFlowSubsystem::CanTransition(EPhase::Login, EPhase::Login));
        TestTrue(TEXT("로비 → 월드 선택"), UP1EntryFlowSubsystem::CanTransition(EPhase::Lobby, EPhase::WorldSelect));
        TestTrue(TEXT("게임 → 월드 선택"), UP1EntryFlowSubsystem::CanTransition(EPhase::Game, EPhase::WorldSelect));
        TestFalse(TEXT("맵 로드 → 월드 선택"), UP1EntryFlowSubsystem::CanTransition(EPhase::MapLoad, EPhase::WorldSelect));
        TestFalse(TEXT("로비 복귀 중 → 월드 선택"), UP1EntryFlowSubsystem::CanTransition(EPhase::ReturningToLobby, EPhase::WorldSelect));
    }

    // 3) 순서를 건너뛰거나 거꾸로 가는 전이는 거절하고 단계를 그대로 둔다.
    {
        UP1EntryFlowSubsystem* Flow = MakeFlow();
        TestFalse(TEXT("로그인 → 로비"), Flow->TryEnterPhase(EPhase::Lobby));
        TestFalse(TEXT("로그인 → 맵 로드"), Flow->TryEnterPhase(EPhase::MapLoad));
        TestFalse(TEXT("로그인 → 게임"), Flow->TryEnterPhase(EPhase::Game));
        TestTrue(TEXT("거절 뒤에도 로그인"), Flow->GetPhase() == EPhase::Login);

        Flow->TryEnterPhase(EPhase::WorldSelect);
        TestFalse(TEXT("월드 선택 → 월드 선택"), Flow->TryEnterPhase(EPhase::WorldSelect));
        TestFalse(TEXT("월드 선택 → 맵 로드"), Flow->TryEnterPhase(EPhase::MapLoad));

        Flow->TryEnterPhase(EPhase::Lobby);
        TestFalse(TEXT("로비 → 로비"), Flow->TryEnterPhase(EPhase::Lobby));
        TestFalse(TEXT("로비 → 게임"), Flow->TryEnterPhase(EPhase::Game));

        Flow->TryEnterPhase(EPhase::MapLoad);
        TestFalse(TEXT("맵 로드 → 로비"), Flow->TryEnterPhase(EPhase::Lobby));
        TestFalse(TEXT("맵 로드 → 맵 로드"), Flow->TryEnterPhase(EPhase::MapLoad));
        TestFalse(TEXT("맵 로드 → 월드 선택"), Flow->TryEnterPhase(EPhase::WorldSelect));

        Flow->TryEnterPhase(EPhase::Game);
        TestFalse(TEXT("게임 → 로비"), Flow->TryEnterPhase(EPhase::Lobby));
        TestFalse(TEXT("게임 → 게임"), Flow->TryEnterPhase(EPhase::Game));
        TestTrue(TEXT("거절 뒤에도 게임"), Flow->GetPhase() == EPhase::Game);

        // 로비 복귀는 게임에서만 시작하고, 저장 뒤 목록을 받아야 로비로 간다. 그사이 게임으로 돌아가거나 맵을 옮기지 않는다.
        UP1EntryFlowSubsystem* ReturnFlow = MakeFlow();
        ReturnFlow->TryEnterPhase(EPhase::WorldSelect);
        ReturnFlow->TryEnterPhase(EPhase::Lobby);
        TestFalse(TEXT("로비 → 로비 복귀 중"), ReturnFlow->TryEnterPhase(EPhase::ReturningToLobby));
        ReturnFlow->TryEnterPhase(EPhase::MapLoad);
        ReturnFlow->TryEnterPhase(EPhase::Game);
        TestTrue(TEXT("게임 → 로비 복귀 중"), ReturnFlow->TryEnterPhase(EPhase::ReturningToLobby));
        TestFalse(TEXT("로비 복귀 중 → 게임"), ReturnFlow->TryEnterPhase(EPhase::Game));
        TestFalse(TEXT("로비 복귀 중 → 맵 로드"), ReturnFlow->TryEnterPhase(EPhase::MapLoad));
        TestTrue(TEXT("로비 복귀 중 → 로비"), ReturnFlow->TryEnterPhase(EPhase::Lobby));
    }

    // 4) 진행 표시는 맵 로드 단계부터 내 플레이어가 스폰될 때까지 「맵을 불러오는 중」이고, 진행이 바뀔 때만 알린다.
    //    스폰 알림은 내 플레이어 데이터가 보내므로 여기서는 로그인으로 돌아가 기다림이 끝나는 경로를 본다.
    {
        using EProgress = EP1EntryProgress;
        UP1EntryFlowSubsystem* Flow = MakeFlow();
        TArray<EProgress> Notified;
        Flow->OnProgressChanged.AddLambda([&Notified](EProgress Progress) { Notified.Add(Progress); });

        TestTrue(TEXT("처음은 진행 없음"), Flow->GetProgress() == EProgress::None);
        Flow->TryEnterPhase(EPhase::WorldSelect);
        Flow->TryEnterPhase(EPhase::Lobby);
        TestEqual(TEXT("월드 선택과 로비에 들어가도 진행이 바뀌지 않으면 알리지 않는다"), Notified.Num(), 0);

        Flow->TryEnterPhase(EPhase::MapLoad);
        TestTrue(TEXT("맵 로드 단계는 맵을 불러오는 중"), Flow->GetProgress() == EProgress::LoadingMap);

        Flow->TryEnterPhase(EPhase::Game);
        TestTrue(TEXT("맵을 열어도 스폰 전이면 맵을 불러오는 중"), Flow->GetProgress() == EProgress::LoadingMap);

        Flow->TryEnterPhase(EPhase::Login);
        TestTrue(TEXT("로그인으로 돌아가면 진행 없음"), Flow->GetProgress() == EProgress::None);

        if (TestEqual(TEXT("알림 수"), Notified.Num(), 2))
        {
            TestTrue(TEXT("첫 알림은 맵을 불러오는 중"), Notified[0] == EProgress::LoadingMap);
            TestTrue(TEXT("둘째 알림은 진행 없음"), Notified[1] == EProgress::None);
        }
    }

    // 5) 로비 복귀를 요청한 뒤 서버의 저장을 기다리는 동안은 「로비로 돌아가는 중」이다. 그사이 게임 맵의 화면이 그대로 보인다.
    //    목록을 받아 로그인 맵을 여는 일은 레벨을 열어야 하므로 여기서 보지 않는다.
    {
        using EProgress = EP1EntryProgress;
        UP1EntryFlowSubsystem* Flow = MakeFlow();
        Flow->TryEnterPhase(EPhase::WorldSelect);
        Flow->TryEnterPhase(EPhase::Lobby);
        Flow->TryEnterPhase(EPhase::MapLoad);
        Flow->TryEnterPhase(EPhase::Game);

        Flow->TryEnterPhase(EPhase::ReturningToLobby);
        TestTrue(TEXT("로비 복귀 중은 로비로 돌아가는 중"), Flow->GetProgress() == EProgress::ReturningToLobby);
    }

    return true;
}

#endif
