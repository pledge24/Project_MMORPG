// 기획 데이터 테이블과 에셋 테이블의 짝 검사가 찾는 문제와 그 심각도를 고정한다.
//
// 왜 이것인가: 엑셀에 아이템이나 몬스터를 더하고 에셋 테이블에 같은 번호의 행을 넣지 않으면, 아이템은 아이콘과 메시가 비고
// 몬스터는 스폰되지 않는다. 남는 흔적은 실행 중의 로그 경고뿐이었다. 짝이 없는 행, 빈 참조, 깨진 참조는 오류로 저장 때 드러나야 하고,
// 기획에서 지운 행이 에셋 테이블에 남은 것은 게임을 깨지 않으므로 경고다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Editor.DesignDataPairCheck

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/P1DesignDataPairTestTables.h"
#include "Validation/P1DesignDataPairCheck.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace P1DesignDataPairCheckTest
{
    using FIssue = FP1DesignDataPairCheck::FIssue;

    /** 심각도가 같고 메시지에 Needle이 든 문제의 수. */
    int32 CountIssues(const TArray<FIssue>& Issues, bool bError, const FString& Needle)
    {
        return Issues.FilterByPredicate([bError, &Needle](const FIssue& Issue)
        {
            return Issue.bError == bError && Issue.Message.Contains(Needle);
        }).Num();
    }

    int32 CountIssues(const TArray<FIssue>& Issues, bool bError)
    {
        return CountIssues(Issues, bError, FString());
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1DesignDataPairCheckTest,
    "P1.Editor.DesignDataPairCheck",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1DesignDataPairCheckTest::RunTest(const FString& Parameters)
{
    using namespace P1DesignDataPairCheckTest;
    using T = FP1DesignDataPairTestTables;
    FP1DesignDataPairTestTables Tables;

    // 1) 아이템. 1000번대는 장비, 2000번대는 장비가 아니다.
    {
        const UDataTable* ItemTable = Tables.MakeDesignTable({ 1000, 1001, 1002, 1003, 1004, 2000, 2001, 2002, 2003 });
        const UDataTable* EquipStatTable = Tables.MakeDesignTable({ 1000, 1001, 1002, 1003, 1004 });
        const UDataTable* AssetTable = Tables.MakeItemAssetTable({
            { 1000, T::ItemAsset(T::Texture, T::SkeletalMesh) },
            { 1001, T::ItemAsset(T::Texture, TEXT(""), T::StaticMesh) },
            { 1002, T::ItemAsset(T::Texture) },
            { 1003, T::ItemAsset(T::Texture, T::Missing) },
            { 1004, T::ItemAsset(T::Missing, T::SkeletalMesh) },
            { 2000, T::ItemAsset(T::Texture) },
            { 2001, T::ItemAsset(TEXT("")) },
            { 2003, T::ItemAsset(T::WrongObject) },
            { 3000, T::ItemAsset(T::Texture) },
        });

        const TArray<FIssue> Issues = FP1DesignDataPairCheck::CheckItems(*ItemTable, *EquipStatTable, *AssetTable);

        // 메시는 스켈레탈 메시와 스태틱 메시 중 하나면 된다. 장비가 아니면 메시가 없어도 된다.
        TestEqual(TEXT("장비의 스켈레탈 메시"), CountIssues(Issues, true, TEXT("1000")), 0);
        TestEqual(TEXT("장비의 스태틱 메시"), CountIssues(Issues, true, TEXT("1001")), 0);
        TestEqual(TEXT("메시 없는 소모품"), CountIssues(Issues, true, TEXT("2000")), 0);

        TestEqual(TEXT("메시가 모두 빈 장비"), CountIssues(Issues, true, TEXT("1002")), 1);
        TestEqual(TEXT("깨진 메시"), CountIssues(Issues, true, TEXT("1003")), 1);
        TestEqual(TEXT("깨진 아이콘"), CountIssues(Issues, true, TEXT("1004")), 1);
        TestEqual(TEXT("빈 아이콘"), CountIssues(Issues, true, TEXT("2001")), 1);
        TestEqual(TEXT("에셋 행이 없는 아이템"), CountIssues(Issues, true, TEXT("2002")), 1);
        TestEqual(TEXT("패키지는 있지만 객체 이름이 틀린 아이콘"), CountIssues(Issues, true, TEXT("2003")), 1);
        TestEqual(TEXT("오류 수"), CountIssues(Issues, true), 6);

        TestEqual(TEXT("기획에서 지운 행은 경고"), CountIssues(Issues, false, TEXT("3000")), 1);
        TestEqual(TEXT("경고 수"), CountIssues(Issues, false), 1);

        // 깨진 참조는 어느 에셋을 가리켰는지 보여 준다.
        TestEqual(TEXT("깨진 경로"), CountIssues(Issues, true, T::Missing), 2);
    }

    // 2) 몬스터. 몬스터 클래스는 블루프린트든 네이티브 클래스든 된다.
    {
        const UDataTable* MonsterTable = Tables.MakeDesignTable({ 5000, 5001, 5002, 5003, 5004 });
        const UDataTable* AssetTable = Tables.MakeMonsterAssetTable({
            { 5000, T::NativeClass },
            { 5001, TEXT("") },
            { 5002, T::MissingClass },
            { 5004, T::NativeClass },
            { 5009, T::NativeClass },
        });

        const TArray<FIssue> Issues = FP1DesignDataPairCheck::CheckMonsters(*MonsterTable, *AssetTable);

        TestEqual(TEXT("빈 몬스터 클래스"), CountIssues(Issues, true, TEXT("5001")), 1);
        TestEqual(TEXT("깨진 몬스터 클래스"), CountIssues(Issues, true, T::MissingClass), 1);
        TestEqual(TEXT("에셋 행이 없는 몬스터"), CountIssues(Issues, true, TEXT("5003")), 1);
        TestEqual(TEXT("오류 수"), CountIssues(Issues, true), 3);
        TestEqual(TEXT("기획에서 지운 행은 경고"), CountIssues(Issues, false, TEXT("5009")), 1);
        TestEqual(TEXT("경고 수"), CountIssues(Issues, false), 1);
    }

    // 3) 짝이 모두 맞으면 문제가 없다.
    {
        const UDataTable* MonsterTable = Tables.MakeDesignTable({ 5000 });
        const UDataTable* AssetTable = Tables.MakeMonsterAssetTable({ { 5000, T::NativeClass } });
        TestEqual(TEXT("문제 없음"), FP1DesignDataPairCheck::CheckMonsters(*MonsterTable, *AssetTable).Num(), 0);
    }

    // 4) 에셋 테이블의 행 구조체가 다르면 행을 읽지 않고 오류 하나로 끝낸다.
    {
        const UDataTable* ItemTable = Tables.MakeDesignTable({ 1000 });
        const UDataTable* WrongTable = Tables.MakeDesignTable({ 1000 });
        const TArray<FIssue> ItemIssues = FP1DesignDataPairCheck::CheckItems(*ItemTable, *ItemTable, *WrongTable);
        TestEqual(TEXT("아이템 에셋 테이블의 행 구조체"), ItemIssues.Num(), 1);
        TestEqual(TEXT("아이템 에셋 테이블의 행 구조체 오류"), CountIssues(ItemIssues, true, TEXT("FP1ItemAssetData")), 1);

        const TArray<FIssue> MonsterIssues = FP1DesignDataPairCheck::CheckMonsters(*ItemTable, *WrongTable);
        TestEqual(TEXT("몬스터 에셋 테이블의 행 구조체"), MonsterIssues.Num(), 1);
        TestEqual(TEXT("몬스터 에셋 테이블의 행 구조체 오류"), CountIssues(MonsterIssues, true, TEXT("FP1MonsterAssetData")), 1);
    }

    return true;
}

#endif
