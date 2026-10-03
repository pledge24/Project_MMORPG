// 일반 공격의 콤보 순번과 몽타주 선택 규칙을 고정한다.
//
// 왜 이것인가: 이 규칙은 전사와 몬스터의 공격 컴포넌트 블루프린트에 따로 있었고, 몬스터 쪽은 서버가 보낸
// 순번을 무시했다(#136). C++ 공격 컴포넌트 하나로 옮기면서 순번 계산과 몽타주 선택을 이 규칙으로 모았다.
//
// 기대값은 규칙을 다시 계산하지 않고 블루프린트의 동작에서 적었다. 몽타주가 4개면 순번은 1→2→3→4→1로 돌고,
// 순번 N은 N−1번째 몽타주를 고른다. 몬스터 공격은 서버가 순번 0을 보내므로 0은 첫 몽타주로 본다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Combat.NormalAttackCombo

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Game/Combat/P1NormalAttackCombo.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1NormalAttackComboTest,
    "P1.Combat.NormalAttackCombo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1NormalAttackComboTest::RunTest(const FString& Parameters)
{
    // 1) 몽타주가 4개면 순번은 1→2→3→4→1로 돈다.
    {
        TestEqual(TEXT("콤보 없음 다음은 1"), FP1NormalAttackCombo::Next(0, 4), 1);
        TestEqual(TEXT("1 다음은 2"), FP1NormalAttackCombo::Next(1, 4), 2);
        TestEqual(TEXT("3 다음은 4"), FP1NormalAttackCombo::Next(3, 4), 4);
        TestEqual(TEXT("4 다음은 1"), FP1NormalAttackCombo::Next(4, 4), 1);
    }

    // 2) 몽타주가 하나면 늘 1이다. 몽타주가 없으면 순번이 생기지 않는다.
    {
        TestEqual(TEXT("몽타주 하나"), FP1NormalAttackCombo::Next(1, 1), 1);
        TestEqual(TEXT("몽타주 없음"), FP1NormalAttackCombo::Next(0, 0), 0);
    }

    // 3) 순번 N은 N−1번째 몽타주를 고른다.
    {
        TestEqual(TEXT("1타"), FP1NormalAttackCombo::MontageIndexFor(1, 4), 0);
        TestEqual(TEXT("4타"), FP1NormalAttackCombo::MontageIndexFor(4, 4), 3);
    }

    // 4) 서버가 보낸 순번 0은 첫 몽타주다. 범위 밖이나 몽타주가 없으면 고르지 않는다.
    {
        TestEqual(TEXT("순번 0"), FP1NormalAttackCombo::MontageIndexFor(0, 1), 0);
        TestEqual(TEXT("범위 밖"), FP1NormalAttackCombo::MontageIndexFor(5, 4), INDEX_NONE);
        TestEqual(TEXT("음수"), FP1NormalAttackCombo::MontageIndexFor(-1, 4), INDEX_NONE);
        TestEqual(TEXT("몽타주 없음"), FP1NormalAttackCombo::MontageIndexFor(0, 0), INDEX_NONE);
    }

    return true;
}

#endif
