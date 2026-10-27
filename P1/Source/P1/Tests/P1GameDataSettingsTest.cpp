// 게임 코드가 기획 데이터를 찾는 창구가 DataBuilder의 데이터 테이블에서 번호로 행을 찾는 규칙을 고정한다.
//
// 왜 이것인가: 클라이언트는 옛 변환기의 JSON을 손으로 쓴 행 구조체로 읽었다. 이제 아이템은 공통, 장비 능력치, 소모 효과
// 세 표로 나뉘고, 몬스터와 함께 생성된 행 구조체의 테이블에서 찾는다. 프로젝트 설정은 에디터 메뉴가 채우는 테이블을 가리킨다.
//
// 기대값은 저장소의 기획 데이터(DesignData/Item.xlsx 등)에서 손으로 옮겨 적었다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Data.GameDataSettings

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/P1DesignDataTestTables.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1GameDataSettingsTest,
    "P1.Data.GameDataSettings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1GameDataSettingsTest::RunTest(const FString& Parameters)
{
    // 1) 프로젝트 설정은 에디터 메뉴가 채우는 기획 데이터 테이블을 가리킨다.
    {
        const UP1GameDataSettings* Settings = GetDefault<UP1GameDataSettings>();
        TestEqual(TEXT("아이템"), Settings->ItemTable.ToSoftObjectPath().ToString(), FString(TEXT("/Game/P1/Data/DesignData/DT_Item.DT_Item")));
        TestEqual(TEXT("장비 능력치"), Settings->ItemEquipStatTable.ToSoftObjectPath().ToString(), FString(TEXT("/Game/P1/Data/DesignData/DT_ItemEquipStat.DT_ItemEquipStat")));
        TestEqual(TEXT("소모 효과"), Settings->ItemConsumeEffectTable.ToSoftObjectPath().ToString(), FString(TEXT("/Game/P1/Data/DesignData/DT_ItemConsumeEffect.DT_ItemConsumeEffect")));
        TestEqual(TEXT("몬스터"), Settings->MonsterTable.ToSoftObjectPath().ToString(), FString(TEXT("/Game/P1/Data/DesignData/DT_Monster.DT_Monster")));
    }

    FP1DesignDataTestTables Tables;

    // 2) 아이템 번호로 공통 행을 찾는다. 종류는 프로토콜 값으로 읽는다.
    {
        const FP1ItemRow* Potion = UP1GameDataSettings::FindItemRow(2000);
        if (TestNotNull(TEXT("아이템 2000"), Potion))
        {
            TestTrue(TEXT("종류"), Potion->GetType() == Protocol::ITEM_TYPE_CONSUMABLE);
            TestEqual(TEXT("구매 가격"), Potion->BuyPrice, int64{ 150 });
        }
        TestNull(TEXT("없는 아이템"), UP1GameDataSettings::FindItemRow(9999));
    }

    // 3) 종류별 표는 그 종류의 아이템에만 행이 있다.
    {
        const FP1ItemEquipStatRow* HoodStat = UP1GameDataSettings::FindItemEquipStatRow(1000);
        if (TestNotNull(TEXT("두건의 장비 능력치"), HoodStat))
            TestTrue(TEXT("부위"), HoodStat->GetPart() == Protocol::GEAR_PART_HELMET);
        TestNull(TEXT("물약의 장비 능력치"), UP1GameDataSettings::FindItemEquipStatRow(2000));

        const FP1ItemConsumeEffectRow* PotionEffect = UP1GameDataSettings::FindItemConsumeEffectRow(2000);
        if (TestNotNull(TEXT("물약의 소모 효과"), PotionEffect))
            TestEqual(TEXT("재사용 대기"), PotionEffect->CooldownMs, 10000);
        TestNull(TEXT("두건의 소모 효과"), UP1GameDataSettings::FindItemConsumeEffectRow(1000));
    }

    // 4) 몬스터 번호로 몬스터 행을 찾는다.
    {
        const FP1MonsterRow* Minion = UP1GameDataSettings::FindMonsterRow(5000);
        if (TestNotNull(TEXT("몬스터 5000"), Minion))
            TestEqual(TEXT("번호"), Minion->Id, 5000);
        TestNull(TEXT("없는 몬스터"), UP1GameDataSettings::FindMonsterRow(9999));
    }

    return true;
}

#endif
