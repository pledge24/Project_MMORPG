// 서버가 알린 맵 번호로 열 레벨을 고르는 규칙을 고정한다.
//
// 왜 이것인가: 클라이언트는 맵 번호와 무관하게 인게임 레벨 하나를 열었다. 맵이 둘 이상이 되면 다른 맵으로 가는 포털과
// 리스폰이 엉뚱한 레벨을 연다. 이제 데이터 설정의 표가 맵 번호를 레벨로 바꾸고, 표에 없는 번호는 열지 않는다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Game.MapLevel

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Game/Data/P1GameDataSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace P1MapLevelTest
{
    /** 프로젝트 설정의 표를 비운 설정. 새로 만든 설정 객체도 프로젝트 설정의 값으로 시작한다. */
    UP1GameDataSettings* MakeEmptySettings()
    {
        UP1GameDataSettings* Settings = NewObject<UP1GameDataSettings>(GetTransientPackage());
        Settings->MapLevels.Reset();
        return Settings;
    }

    constexpr int32 TOWN_MAP_ID = 1111;
    constexpr int32 DUNGEON_MAP_ID = 2222;
    constexpr int32 UNKNOWN_MAP_ID = 9999;

    const TCHAR* const TOWN_LEVEL_PATH = TEXT("/Game/P1/Maps/L_InGameMap.L_InGameMap");
    const TCHAR* const DUNGEON_LEVEL_PATH = TEXT("/Game/P1/Maps/L_Dungeon.L_Dungeon");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1MapLevelTest,
    "P1.Game.MapLevel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1MapLevelTest::RunTest(const FString& Parameters)
{
    using namespace P1MapLevelTest;

    // 1) 맵 번호마다 그 번호의 레벨을 고른다.
    {
        UP1GameDataSettings* Settings = MakeEmptySettings();
        Settings->MapLevels.Add(TOWN_MAP_ID, TSoftObjectPtr<UWorld>(FSoftObjectPath(TOWN_LEVEL_PATH)));
        Settings->MapLevels.Add(DUNGEON_MAP_ID, TSoftObjectPtr<UWorld>(FSoftObjectPath(DUNGEON_LEVEL_PATH)));

        TestEqual(TEXT("마을 맵 번호"), Settings->FindMapLevel(TOWN_MAP_ID).ToSoftObjectPath().ToString(), FString(TOWN_LEVEL_PATH));
        TestEqual(TEXT("던전 맵 번호"), Settings->FindMapLevel(DUNGEON_MAP_ID).ToSoftObjectPath().ToString(), FString(DUNGEON_LEVEL_PATH));
    }

    // 2) 표에 없는 맵 번호는 레벨을 고르지 않는다. 접속 흐름은 이것을 보고 연결을 끊는다.
    {
        UP1GameDataSettings* Settings = MakeEmptySettings();
        Settings->MapLevels.Add(TOWN_MAP_ID, TSoftObjectPtr<UWorld>(FSoftObjectPath(TOWN_LEVEL_PATH)));

        TestTrue(TEXT("모르는 맵 번호"), Settings->FindMapLevel(UNKNOWN_MAP_ID).IsNull());
        TestTrue(TEXT("빈 표"), MakeEmptySettings()->FindMapLevel(TOWN_MAP_ID).IsNull());
    }

    // 3) 프로젝트 설정의 표는 서버 맵 표의 맵을 인게임 레벨로 연다.
    {
        TestEqual(TEXT("프로젝트 설정의 마을 맵"),
            GetDefault<UP1GameDataSettings>()->FindMapLevel(TOWN_MAP_ID).ToSoftObjectPath().ToString(), FString(TOWN_LEVEL_PATH));
    }

    return true;
}

#endif
