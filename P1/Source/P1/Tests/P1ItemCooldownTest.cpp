// 아이템 하나의 재사용 대기 계산(남은 시간과 비율)을 고정한다.
//
// 왜 이것인가: 슬롯의 대기 막대가 이 계산으로 그려지고, 대기 중인 소모품의 사용 요청을 막는 판정도 이 계산을
// 쓴다(#133). 블루프린트는 0.05초마다 경과 시간을 더해 막대를 1에서 0으로 줄였다.
//
// 기대값은 계산을 다시 하지 않고 손으로 적었다. 소모품의 대기는 10초다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Inventory.ItemCooldown

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Game/Inventory/P1ItemCooldown.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1ItemCooldownTest,
    "P1.Inventory.ItemCooldown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1ItemCooldownTest::RunTest(const FString& Parameters)
{
    const FP1ItemCooldown Potion{ 100.0, 10.0 };

    // 1) 남은 시간은 시작 시각부터 줄고, 끝나면 0에 머문다.
    {
        TestEqual(TEXT("시작 직후"), Potion.GetRemainingSeconds(100.0), 10.0, UE_DOUBLE_KINDA_SMALL_NUMBER);
        TestEqual(TEXT("중간"), Potion.GetRemainingSeconds(102.5), 7.5, UE_DOUBLE_KINDA_SMALL_NUMBER);
        TestEqual(TEXT("정확히 끝난 순간"), Potion.GetRemainingSeconds(110.0), 0.0, UE_DOUBLE_KINDA_SMALL_NUMBER);
        TestEqual(TEXT("끝난 뒤"), Potion.GetRemainingSeconds(125.0), 0.0, UE_DOUBLE_KINDA_SMALL_NUMBER);
    }

    // 2) 막대에 쓰는 비율은 남은 비율이다. 1에서 시작해 0으로 줄어든다.
    {
        TestEqual(TEXT("시작 직후 비율"), Potion.GetRemainingRatio(100.0), 1.f, KINDA_SMALL_NUMBER);
        TestEqual(TEXT("중간 비율"), Potion.GetRemainingRatio(102.5), 0.75f, KINDA_SMALL_NUMBER);
        TestEqual(TEXT("끝난 뒤 비율"), Potion.GetRemainingRatio(125.0), 0.f, KINDA_SMALL_NUMBER);
    }

    // 3) 남은 시간이 있을 때만 대기 중이다. 정확히 끝난 순간은 대기 중이 아니다.
    {
        TestTrue(TEXT("시작 직후 대기 중"), Potion.IsCoolingDown(100.0));
        TestTrue(TEXT("끝나기 직전 대기 중"), Potion.IsCoolingDown(109.9));
        TestFalse(TEXT("정확히 끝난 순간"), Potion.IsCoolingDown(110.0));
    }

    // 4) 길이가 0 이하면 대기가 없다. 장비의 데이터 값은 -1이다.
    {
        const FP1ItemCooldown None{ 100.0, 0.0 };
        TestFalse(TEXT("길이 0은 대기 중이 아니다"), None.IsCoolingDown(100.0));
        TestEqual(TEXT("길이 0의 비율"), None.GetRemainingRatio(100.0), 0.f, KINDA_SMALL_NUMBER);

        const FP1ItemCooldown Gear{ 100.0, -1.0 };
        TestFalse(TEXT("길이 -1은 대기 중이 아니다"), Gear.IsCoolingDown(100.0));
        TestEqual(TEXT("길이 -1의 비율"), Gear.GetRemainingRatio(100.0), 0.f, KINDA_SMALL_NUMBER);
    }

    return true;
}

#endif
