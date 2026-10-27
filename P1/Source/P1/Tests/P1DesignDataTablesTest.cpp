// DataBuilder가 만든 클라이언트 JSON이 생성된 행 구조체의 데이터 테이블로 그대로 채워지는지 고정한다.
//
// 왜 이것인가: 에디터 메뉴(Scripts/Editor/p1_design_data_tables.py)는 엔진의 JSON 임포터로 테이블을 비우고 다시 채운다.
// JSON의 필드와 행 구조체의 속성이 어긋나면 임포터는 문제를 로그에만 남기고 일부 값을 기본값으로 둔다.
// 여기서는 에셋을 만들지 않고 임시 테이블에 같은 임포터를 돌린다. 행 이름은 JSON의 RowName(기획 데이터의 키)이다.
//
// 기대값은 저장소의 기획 데이터(DesignData/Item.xlsx 등)에서 손으로 옮겨 적었다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Data.DesignDataTables

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/DataTable.h"
#include "Game/Data/Generated/P1AreaRow.h"
#include "Game/Data/Generated/P1ItemConsumeEffectRow.h"
#include "Game/Data/Generated/P1ItemEquipStatRow.h"
#include "Game/Data/Generated/P1ItemRow.h"
#include "Game/Data/Generated/P1MapRow.h"
#include "Game/Data/Generated/P1MonsterRow.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace P1DesignDataTablesTest
{
    FString ReadGeneratedJson(const FString& TableName)
    {
        FString Json;
        FFileHelper::LoadFileToString(Json, *FPaths::Combine(FPaths::ProjectDir(), TEXT("DesignData/Generated"), TableName + TEXT(".json")));
        return Json;
    }

    /** 에디터 메뉴와 같은 설정으로 새 임시 테이블을 만들어 채운다. 임포터가 알린 문제는 OutProblems에 쓴다. */
    UDataTable* Fill(UScriptStruct* RowStruct, const FString& Json, TArray<FString>* OutProblems = nullptr)
    {
        // 행이 있는 테이블의 RowStruct를 바꾸면 비울 때 다른 구조체로 행을 해제한다. 표마다 새 테이블을 쓴다.
        UDataTable* Table = NewObject<UDataTable>(GetTransientPackage());
        Table->RowStruct = RowStruct;
        Table->ImportKeyField = TEXT("RowName");
        const TArray<FString> Problems = Table->CreateTableFromJSONString(Json);
        if (OutProblems != nullptr)
            *OutProblems = Problems;
        return Table;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1DesignDataTablesTest,
    "P1.Data.DesignDataTables",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1DesignDataTablesTest::RunTest(const FString& Parameters)
{
    using namespace P1DesignDataTablesTest;

    // 1) 저장소의 클라이언트 JSON이 모두 문제 없이 채워진다. 행 수는 JSON의 행 수다.
    {
        const TArray<TPair<FString, UScriptStruct*>> Tables = {
            { TEXT("Item"), FP1ItemRow::StaticStruct() },
            { TEXT("ItemEquipStat"), FP1ItemEquipStatRow::StaticStruct() },
            { TEXT("ItemConsumeEffect"), FP1ItemConsumeEffectRow::StaticStruct() },
            { TEXT("Monster"), FP1MonsterRow::StaticStruct() },
            { TEXT("Map"), FP1MapRow::StaticStruct() },
            { TEXT("Area"), FP1AreaRow::StaticStruct() },
        };
        for (const TPair<FString, UScriptStruct*>& Entry : Tables)
        {
            const FString Json = ReadGeneratedJson(Entry.Key);
            if (TestFalse(FString::Printf(TEXT("%s.json이 있음"), *Entry.Key), Json.IsEmpty()) == false)
                continue;

            TArray<FString> Problems;
            const UDataTable* Table = Fill(Entry.Value, Json, &Problems);
            TestEqual(FString::Printf(TEXT("%s 임포트 문제: %s"), *Entry.Key, *FString::Join(Problems, TEXT(" / "))), Problems.Num(), 0);

            int32 JsonRows = 0;
            for (const TCHAR* Cursor = *Json; (Cursor = FCString::Strstr(Cursor, TEXT("\"RowName\""))) != nullptr; ++Cursor)
                ++JsonRows;
            TestEqual(FString::Printf(TEXT("%s 행 수"), *Entry.Key), Table->GetRowMap().Num(), JsonRows);
        }
    }

    // 2) 아이템 행: 행 이름은 아이템 번호이고, .proto 열거형은 값 이름으로 담겨 프로토콜 값으로 읽힌다.
    {
        const UDataTable* Table = Fill(FP1ItemRow::StaticStruct(), ReadGeneratedJson(TEXT("Item")));
        const FP1ItemRow* Hood = Table->FindRow<FP1ItemRow>(TEXT("1000"), TEXT("테스트"));
        if (TestNotNull(TEXT("행 1000"), Hood))
        {
            TestEqual(TEXT("번호"), Hood->Id, 1000);
            TestEqual(TEXT("이름"), Hood->Name, FString(TEXT("초보자의 두건")));
            TestTrue(TEXT("종류"), Hood->GetType() == Protocol::ITEM_TYPE_GEAR);
            TestEqual(TEXT("착용 직업 수"), Hood->GetAllowedClasses().Num(), 1);
            if (Hood->GetAllowedClasses().Num() == 1)
                TestTrue(TEXT("착용 직업"), Hood->GetAllowedClasses()[0] == Protocol::CHARACTER_CLASS_WARRIOR);
            TestEqual(TEXT("구매 가격"), Hood->BuyPrice, int64{ 100 });
            TestTrue(TEXT("판매 가능"), Hood->Sellable);
        }
    }

    // 3) 장비 능력치 행의 부위, 지역 행의 생성된 열거형과 중첩된 포털.
    {
        const UDataTable* StatTable = Fill(FP1ItemEquipStatRow::StaticStruct(), ReadGeneratedJson(TEXT("ItemEquipStat")));
        const FP1ItemEquipStatRow* HoodStat = StatTable->FindRow<FP1ItemEquipStatRow>(TEXT("1000"), TEXT("테스트"));
        if (TestNotNull(TEXT("장비 능력치 1000"), HoodStat))
            TestTrue(TEXT("부위"), HoodStat->GetPart() == Protocol::GEAR_PART_HELMET);

        const UDataTable* AreaTable = Fill(FP1AreaRow::StaticStruct(), ReadGeneratedJson(TEXT("Area")));
        const FP1AreaRow* Town = AreaTable->FindRow<FP1AreaRow>(TEXT("10"), TEXT("테스트"));
        if (TestNotNull(TEXT("지역 10"), Town))
        {
            TestTrue(TEXT("지역 유형"), Town->Type == EP1AreaType::SAFE_ZONE);
            TestTrue(TEXT("마을"), Town->HasRespawnPoint);
            TestEqual(TEXT("마을의 포털 수"), Town->Portals.Num(), 3);
            if (Town->Portals.Num() == 3)
                TestEqual(TEXT("첫 포털"), Town->Portals[0].Id, 11);
        }
    }

    // 4) 다시 채우면 테이블을 비우고 채운다. JSON에서 빠진 행은 남지 않는다.
    {
        UDataTable* Table = Fill(FP1MapRow::StaticStruct(), TEXT("[{\"RowName\":\"1\",\"Id\":1,\"Name\":\"A\"},{\"RowName\":\"2\",\"Id\":2,\"Name\":\"B\"}]"));
        TestEqual(TEXT("처음 행 수"), Table->GetRowMap().Num(), 2);

        Table->CreateTableFromJSONString(TEXT("[{\"RowName\":\"2\",\"Id\":2,\"Name\":\"B2\"}]"));
        TestEqual(TEXT("다시 채운 행 수"), Table->GetRowMap().Num(), 1);
        TestNull(TEXT("지운 행"), Table->FindRow<FP1MapRow>(TEXT("1"), TEXT("테스트"), false));
        const FP1MapRow* Kept = Table->FindRow<FP1MapRow>(TEXT("2"), TEXT("테스트"));
        if (TestNotNull(TEXT("남은 행"), Kept))
            TestEqual(TEXT("남은 행의 새 값"), Kept->Name, FString(TEXT("B2")));
    }

    // 5) 값 이름이 아니면 0번 값이다. 셀의 값 이름은 대문자뿐이라 대소문자를 구분한다.
    {
        FP1ItemRow Row;
        Row.Type = TEXT("gear");
        TestTrue(TEXT("소문자 값 이름"), Row.GetType() == Protocol::ITEM_TYPE_NONE);
        Row.Type = FString();
        TestTrue(TEXT("빈 값"), Row.GetType() == Protocol::ITEM_TYPE_NONE);
        Row.AllowedClasses = { TEXT("WARRIOR"), TEXT("ROGUE") };
        const TArray<Protocol::CharacterClass> Classes = Row.GetAllowedClasses();
        if (TestEqual(TEXT("원소 수"), Classes.Num(), 2))
        {
            TestTrue(TEXT("아는 값"), Classes[0] == Protocol::CHARACTER_CLASS_WARRIOR);
            TestTrue(TEXT("모르는 값"), Classes[1] == Protocol::CHARACTER_CLASS_NONE);
        }
    }

    return true;
}

#endif
