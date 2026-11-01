// 내 플레이어의 일반 공격 콤보가 언제 어느 액션으로 이어지는지 고정한다.
//
// 왜 이것인가: 콤보는 단일 몽타주의 섹션 넷이고, 다음 단계는 액션 표의 NextAction이 정한다. 콤보 입력 창(노티파이
// 스테이트) 안에서 누른 입력만 다음 단계로 이어지고, 다음 단계는 창이 닫힐 때 시작한다. 창이 닫히기 전에 넘어가면
// 지금 단계의 타격 뒤 동작이 잘리고, 섹션 끝까지 기다리면 회복 동작을 다 본 뒤에야 다음 단계가 나간다. 입력이 없으면
// 그 단계의 남은 동작을 끝까지 재생하고 콤보가 끝난다. 예전에는 마지막 공격 뒤 2초 동안 입력이 없을 때 처음으로 돌아갔다.
//
// 기대값은 규칙에서 적었다. 다음 단계 조회는 가짜(1→2→3→4→없음)로 바꿔 끼우고, 마지막 사례만 저장소의 기획 데이터
// (직업 표의 NormalAttackAction과 액션 표의 NextAction)를 읽는다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Combat.ActionCombo

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Game/Combat/P1ActionCombo.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/Generated/P1ActionRow.h"
#include "Game/Data/Generated/P1ClassRow.h"
#include "Tests/P1DesignDataTestTables.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace P1ActionComboTest
{
    /** 1→2→3→4로 이어지고 4에서 끝나는 콤보. */
    int32 FindNextAction(int32 ActionId)
    {
        return ActionId >= 1 && ActionId < 4 ? ActionId + 1 : 0;
    }

    constexpr int32 FIRST_ACTION = 1;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1ActionComboTest,
    "P1.Combat.ActionCombo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1ActionComboTest::RunTest(const FString& Parameters)
{
    using namespace P1ActionComboTest;

    // 1) 진행 중인 액션이 없으면 입력이 첫 단계를 바로 시작한다.
    {
        FP1ActionCombo Combo(&FindNextAction);
        TestFalse(TEXT("처음에는 진행 중인 액션이 없다"), Combo.IsActing());
        TestEqual(TEXT("첫 입력은 첫 단계"), Combo.Input(FIRST_ACTION), 1);
        TestEqual(TEXT("진행 중인 액션"), Combo.GetCurrentAction(), 1);
    }

    // 2) 입력 창 안의 입력은 다음 단계를 예약하고, 창이 닫힐 때 시작한다. NextAction을 따라 마지막 단계까지 간다.
    {
        FP1ActionCombo Combo(&FindNextAction);
        Combo.Input(FIRST_ACTION);
        for (int32 Expected = 2; Expected <= 4; Expected++)
        {
            Combo.BeginWindow();
            TestEqual(TEXT("창 안의 입력은 바로 시작하지 않는다"), Combo.Input(FIRST_ACTION), 0);
            TestEqual(TEXT("창이 닫히기 전에는 지금 단계"), Combo.GetCurrentAction(), Expected - 1);
            TestEqual(FString::Printf(TEXT("창이 닫히면 %d단계"), Expected), Combo.EndWindow(), Expected);
            TestEqual(TEXT("진행 중인 액션이 바뀐다"), Combo.GetCurrentAction(), Expected);
        }
    }

    // 3) 입력 창 밖의 입력은 무시한다. 창이 열리기 전에 누른 입력도, 창이 닫힌 뒤에 누른 입력도 다음 단계로 이어지지 않는다.
    {
        FP1ActionCombo Combo(&FindNextAction);
        Combo.Input(FIRST_ACTION);
        TestEqual(TEXT("창이 열리기 전의 입력"), Combo.Input(FIRST_ACTION), 0);
        Combo.BeginWindow();
        TestEqual(TEXT("창 안에서 누르지 않으면 이어지지 않는다"), Combo.EndWindow(), 0);
        TestEqual(TEXT("창이 닫힌 뒤의 입력"), Combo.Input(FIRST_ACTION), 0);
        TestEqual(TEXT("지금 단계의 남은 동작을 재생한다"), Combo.GetCurrentAction(), 1);
    }

    // 4) 입력이 없으면 섹션이 끝난 뒤 콤보가 끝나고, 다음 입력은 첫 단계부터다.
    {
        FP1ActionCombo Combo(&FindNextAction);
        Combo.Input(FIRST_ACTION);
        Combo.BeginWindow();
        TestEqual(TEXT("창에서 입력이 없으면 다음 단계가 없다"), Combo.EndWindow(), 0);
        Combo.EndAction();
        TestFalse(TEXT("섹션이 끝나면 콤보가 끝난다"), Combo.IsActing());
        TestEqual(TEXT("다음 입력은 첫 단계부터"), Combo.Input(FIRST_ACTION), 1);
    }

    // 5) 마지막 단계에는 다음 단계가 없다. 창이 열려도 입력은 예약되지 않는다.
    {
        FP1ActionCombo Combo([](int32 ActionId) { return 0; });
        Combo.Input(FIRST_ACTION);
        Combo.BeginWindow();
        Combo.Input(FIRST_ACTION);
        TestEqual(TEXT("다음 단계가 없으면 이어지지 않는다"), Combo.EndWindow(), 0);
    }

    // 6) 저장소의 기획 데이터에서 전사의 콤보는 직업의 첫 단계에서 시작해 NextAction을 따라 네 단계다.
    {
        FP1DesignDataTestTables Tables;
        const FP1ClassRow* Warrior = UP1GameDataSettings::FindClassRow(Protocol::CHARACTER_CLASS_WARRIOR);
        if (TestNotNull(TEXT("직업 표의 전사"), Warrior))
        {
            FP1ActionCombo Combo([](int32 ActionId)
            {
                const FP1ActionRow* Row = UP1GameDataSettings::FindActionRow(ActionId);
                return Row ? Row->NextAction : 0;
            });

            TArray<int32> Steps = { Combo.Input(Warrior->NormalAttackAction) };
            for (int32 Guard = 0; Guard < 10; Guard++)
            {
                Combo.BeginWindow();
                Combo.Input(Warrior->NormalAttackAction);
                const int32 Next = Combo.EndWindow();
                if (Next == 0)
                    break;
                Steps.Add(Next);
            }
            TestEqual(TEXT("전사 콤보"), Steps, TArray<int32>({ 1001, 1002, 1003, 1004 }));
        }
    }

    return true;
}

#endif
