// 에셋 검증기가 프로젝트 설정의 수치 테이블과 에셋 테이블 어느 쪽을 검증해도 짝 검사를 돌리고, 결과를 검증 결과로 싣는 것을 고정한다.
//
// 왜 이것인가: 저장할 때 도는 검증은 저장하는 테이블 하나만 받는다. 에셋 테이블의 행을 비우고 저장해도, 수치 테이블에 행을 더하고
// 저장해도 오류가 보여야 한다. 테이블 경로는 코드에 적지 않고 프로젝트 설정 「P1 Game Data」에서 읽는다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Editor.DesignDataPairValidator

#include "CoreMinimal.h"
#include "Editor.h"
#include "EditorValidatorSubsystem.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "Tests/P1DesignDataPairTestTables.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace P1DesignDataPairValidatorTest
{
    /** 살아 있는 동안 프로젝트 설정의 테이블 하나를 바꾸고, 사라질 때 되돌린다. */
    class FScopedSettingTable
    {
    public:
        FScopedSettingTable(TSoftObjectPtr<UDataTable>& InSlot, UDataTable* Table)
            : Slot(InSlot)
            , Original(InSlot)
        {
            Slot = Table;
        }

        ~FScopedSettingTable()
        {
            Slot = Original;
        }

    private:
        TSoftObjectPtr<UDataTable>& Slot;
        TSoftObjectPtr<UDataTable> Original;
    };

    struct FResult
    {
        EDataValidationResult Result = EDataValidationResult::NotValidated;
        TArray<FText> Warnings;
        TArray<FText> Errors;
    };

    FResult Validate(UObject* Object)
    {
        FResult Out;
        FDataValidationContext Context(false, EDataValidationUsecase::Manual, {});
        Out.Result = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>()->IsObjectValidWithContext(Object, Context);
        Context.SplitIssues(Out.Warnings, Out.Errors);
        return Out;
    }

    bool AnyContains(const TArray<FText>& Messages, const FString& Needle)
    {
        return Messages.ContainsByPredicate([&Needle](const FText& Message) { return Message.ToString().Contains(Needle); });
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1DesignDataPairValidatorTest,
    "P1.Editor.DesignDataPairValidator",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1DesignDataPairValidatorTest::RunTest(const FString& Parameters)
{
    using namespace P1DesignDataPairValidatorTest;
    using T = FP1DesignDataPairTestTables;
    FP1DesignDataPairTestTables Tables;
    UP1GameDataSettings* Settings = GetMutableDefault<UP1GameDataSettings>();

    // 1) 아이템 2000의 에셋 행이 없다. 수치 테이블, 장비 능력치 테이블, 에셋 테이블 어느 쪽을 검증해도 오류다.
    {
        UDataTable* ItemTable = Tables.MakeDesignTable({ 1000, 2000 });
        UDataTable* AssetTable = Tables.MakeItemAssetTable({ { 1000, T::ItemAsset(T::Texture, T::SkeletalMesh) } });
        FScopedSettingTable Item(Settings->ItemTable, ItemTable);
        UDataTable* EquipStatTable = Tables.MakeDesignTable({ 1000 });
        FScopedSettingTable EquipStat(Settings->ItemEquipStatTable, EquipStatTable);
        FScopedSettingTable Asset(Settings->ItemAssetTable, AssetTable);

        for (UDataTable* Validated : { ItemTable, EquipStatTable, AssetTable })
        {
            const FResult Result = Validate(Validated);
            TestTrue(TEXT("아이템 짝 오류는 실패"), Result.Result == EDataValidationResult::Invalid);
            TestTrue(TEXT("아이템 짝 오류의 메시지"), AnyContains(Result.Errors, TEXT("2000")));
        }
    }

    // 2) 몬스터 5009는 기획에서 지운 행이다. 경고만 있으면 통과한다.
    {
        UDataTable* MonsterTable = Tables.MakeDesignTable({ 5000 });
        UDataTable* AssetTable = Tables.MakeMonsterAssetTable({
            { 5000, T::MonsterAsset(T::SkeletalMesh, T::NativeAnimClass) },
            { 5009, T::MonsterAsset(T::SkeletalMesh, T::NativeAnimClass) },
        });
        FScopedSettingTable Monster(Settings->MonsterTable, MonsterTable);
        FScopedSettingTable Asset(Settings->MonsterAssetTable, AssetTable);

        const FResult Result = Validate(AssetTable);
        TestTrue(TEXT("경고만 있으면 통과"), Result.Result == EDataValidationResult::Valid);
        TestEqual(TEXT("오류 없음"), Result.Errors.Num(), 0);
        TestTrue(TEXT("기획에서 지운 행의 경고"), AnyContains(Result.Warnings, TEXT("5009")));
    }

    // 3) 짝이 되는 테이블을 불러오지 못하면 어느 설정인지 알리고 실패한다.
    {
        UDataTable* MonsterTable = Tables.MakeDesignTable({ 5000 });
        FScopedSettingTable Monster(Settings->MonsterTable, MonsterTable);
        FScopedSettingTable Asset(Settings->MonsterAssetTable, nullptr);

        const FResult Result = Validate(MonsterTable);
        TestTrue(TEXT("짝 테이블이 없으면 실패"), Result.Result == EDataValidationResult::Invalid);
        TestTrue(TEXT("빠진 설정 이름"), AnyContains(Result.Errors, TEXT("MonsterAssetTable")));
    }

    // 4) 프로젝트 설정이 가리키지 않는 테이블은 짝 검사를 하지 않는다.
    {
        UDataTable* Unrelated = Tables.MakeMonsterAssetTable({ { 5001, T::MonsterAsset(TEXT(""), TEXT("")) } });
        const FResult Result = Validate(Unrelated);
        TestTrue(TEXT("설정 밖의 테이블은 실패하지 않음"), Result.Result != EDataValidationResult::Invalid);
        TestEqual(TEXT("설정 밖의 테이블은 오류 없음"), Result.Errors.Num(), 0);
    }

    return true;
}

#endif
