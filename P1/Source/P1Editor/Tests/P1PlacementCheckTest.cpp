// 배치 추출의 검사와 산출물 형식을 고정한다.
//
// 왜 이것인가: 지역의 경계와 포털 좌표를 엑셀에 숫자로 적던 때는 레벨에 놓인 것과 표의 숫자를 사람이 손으로 맞췄다.
// 레벨에서 추출하면 맞출 일은 없지만, 겹친 지역이나 지역 밖의 포털, 없는 기획 데이터 번호를 그대로 서버에 넘기면
// 룸 배정과 포털 판정이 조용히 틀린다. 추출은 이런 배치를 0이 아닌 종료 코드로 막아야 한다.
// 산출물은 git에 커밋하므로, 배치 하나를 옮기면 그 행 한 줄만 바뀌어야 한다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Editor.PlacementCheck

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Placement/P1PlacementCheck.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace P1PlacementCheckTest
{
    constexpr int32 MAP_ID = 1111;
    constexpr int32 TOWN_ID = 10;
    constexpr int32 FIELD_ID = 20;

    FP1PlacementCheck::FPlacedBox MakeBox(const FString& Name, double CenterX, double CenterY, double HalfExtentX, double HalfExtentY)
    {
        FP1PlacementCheck::FPlacedBox Box;
        Box.Name = Name;
        Box.Center = FVector(CenterX, CenterY, 0.0);
        Box.HalfExtent = FVector2D(HalfExtentX, HalfExtentY);
        return Box;
    }

    FP1PlacementCheck::FPlacedPoint MakePoint(const FString& Name, int32 Id, double X, double Y, double Yaw = 0.0, double Radius = 0.0)
    {
        FP1PlacementCheck::FPlacedPoint Point;
        Point.Name = Name;
        Point.Id = Id;
        Point.Location = FVector(X, Y, 0.0);
        Point.Yaw = Yaw;
        Point.Radius = Radius;
        return Point;
    }

    FP1PlacementCheck::FPlacedWall MakeWall(const FString& Name, int32 AreaId, double CenterX, double CenterY, double HalfExtent)
    {
        FP1PlacementCheck::FPlacedWall Wall;
        Wall.Name = Name;
        Wall.AreaId = AreaId;
        Wall.Center = FVector(CenterX, CenterY, 0.0);
        Wall.HalfExtent = FVector2D(HalfExtent, HalfExtent);
        return Wall;
    }

    /** 마을과 사냥터 하나. 두 지역을 잇는 포털이 하나씩 있고, 마을에 리스폰 지점이 있다. */
    FP1PlacementCheck::FLevel MakeLevel()
    {
        FP1PlacementCheck::FLevel Level;
        Level.MapId = MAP_ID;
        Level.LevelName = TEXT("L_Test");

        FP1PlacementCheck::FPlacedMap Map;
        Map.Bounds = MakeBox(TEXT("MapMarker"), -5000.0, 0.0, 8000.0, 13000.0);
        Map.CellSize = 1000.f;
        Level.MapMarkers.Add(Map);

        Level.Areas.Add({ TOWN_ID, MakeBox(TEXT("TownVolume"), 0.0, 0.0, 1500.0, 1500.0) });
        Level.Areas.Add({ FIELD_ID, MakeBox(TEXT("FieldVolume"), -10000.0, 10000.0, 2500.0, 2500.0) });

        // 소수점 둘째 자리까지 반올림한 값이 산출물에 들어간다.
        Level.RespawnPoints.Add(MakePoint(TEXT("TownRespawn"), 0, 0.004, 0.0));
        Level.Portals.Add(MakePoint(TEXT("TownPortal"), 11, -1199.996, 1000.0, 0.0, 500.0));
        Level.Portals.Add(MakePoint(TEXT("FieldPortal"), 21, -7750.0, 10000.0, 0.0, 500.0));
        Level.Arrivals.Add(MakePoint(TEXT("FieldArrival"), 11, -8000.0, 10000.0, 180.0));
        Level.Arrivals.Add(MakePoint(TEXT("TownArrival"), 21, -1000.0, 1000.0, 0.0));

        Level.Walls.Add(MakeWall(TEXT("TownWall"), TOWN_ID, 0.0, 0.0, 1500.0));
        Level.Walls.Add(MakeWall(TEXT("FieldWall"), FIELD_ID, -10000.0, 10000.0, 2500.0));
        return Level;
    }

    FP1PlacementCheck::FDesignData MakeDesign()
    {
        FP1PlacementCheck::FDesignData Design;
        Design.MapIds.Add(MAP_ID);
        Design.Areas.Add(TOWN_ID, { MAP_ID, true });
        Design.Areas.Add(FIELD_ID, { MAP_ID, false });
        Design.Portals.Add(11, { TOWN_ID, FIELD_ID });
        Design.Portals.Add(21, { FIELD_ID, TOWN_ID });
        return Design;
    }

    const TCHAR* EXPECTED_FILE =
        TEXT("{\"MapId\":1111,\"CenterX\":-5000.0,\"CenterY\":0.0,\"HalfExtentX\":8000.0,\"HalfExtentY\":13000.0,\"CellSize\":1000.0,\n")
        TEXT("\"Areas\":[\n")
        TEXT("{\"Id\":10,\"CenterX\":0.0,\"CenterY\":0.0,\"CenterZ\":0.0,\"HalfExtentX\":1500.0,\"HalfExtentY\":1500.0},\n")
        TEXT("{\"Id\":20,\"CenterX\":-10000.0,\"CenterY\":10000.0,\"CenterZ\":0.0,\"HalfExtentX\":2500.0,\"HalfExtentY\":2500.0}\n")
        TEXT("],\n")
        TEXT("\"RespawnPoints\":[\n")
        TEXT("{\"AreaId\":10,\"X\":0.0,\"Y\":0.0,\"Z\":0.0,\"Yaw\":0.0}\n")
        TEXT("],\n")
        TEXT("\"Portals\":[\n")
        TEXT("{\"Id\":11,\"SrcX\":-1200.0,\"SrcY\":1000.0,\"SrcZ\":0.0,\"Radius\":500.0,\"DestX\":-8000.0,\"DestY\":10000.0,\"DestZ\":0.0,\"DestYaw\":180.0},\n")
        TEXT("{\"Id\":21,\"SrcX\":-7750.0,\"SrcY\":10000.0,\"SrcZ\":0.0,\"Radius\":500.0,\"DestX\":-1000.0,\"DestY\":1000.0,\"DestZ\":0.0,\"DestYaw\":0.0}\n")
        TEXT("]}\n");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1PlacementCheckTest,
    "P1.Editor.PlacementCheck",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1PlacementCheckTest::RunTest(const FString& Parameters)
{
    using namespace P1PlacementCheckTest;

    // 오류 하나만 있게 고친 레벨을 검사하고, 0이 아닌 종료 코드와 메시지의 단서를 확인한다.
    auto ExpectRejected = [this](const FString& What, const FP1PlacementCheck::FLevel& Level, const FP1PlacementCheck::FDesignData& Design,
        const TArray<FString>& Needles)
    {
        const FP1PlacementCheck::FResult Result = FP1PlacementCheck::Run({ Level }, Design);
        TestNotEqual(What + TEXT(": 종료 코드"), Result.GetExitCode(), 0);
        TestEqual(What + TEXT(": 오류가 있으면 산출물을 만들지 않는다"), Result.Files.Num(), 0);
        const FString Joined = FString::Join(Result.Errors, TEXT("\n"));
        for (const FString& Needle : Needles)
            TestTrue(FString::Printf(TEXT("%s: 오류에 '%s'가 있다. 오류: %s"), *What, *Needle, *Joined), Joined.Contains(Needle));
    };

    // 1) 올바른 배치는 통과하고, 맵 하나에 한 행 한 줄의 파일 하나를 만든다.
    {
        const FP1PlacementCheck::FResult Result = FP1PlacementCheck::Run({ MakeLevel() }, MakeDesign());
        TestEqual(TEXT("올바른 배치의 종료 코드"), Result.GetExitCode(), 0);
        TestEqual(TEXT("오류 없음"), FString::Join(Result.Errors, TEXT("\n")), FString());
        TestTrue(TEXT("맵 1111의 산출물"), Result.Files.Contains(MAP_ID));
        if (const FString* File = Result.Files.Find(MAP_ID))
            TestEqual(TEXT("산출물 형식"), *File, FString(EXPECTED_FILE));
    }

    // 2) 지역끼리 겹친다. 경계를 함께 쓰는 것은 겹친 것이 아니다.
    {
        FP1PlacementCheck::FLevel Level = MakeLevel();
        // 사냥터를 마을의 서쪽에 붙인다. 마을의 x 범위는 -1500~1500이다.
        Level.Areas[1].Box = MakeBox(TEXT("FieldVolume"), -2000.0, 0.0, 1000.0, 1000.0);
        Level.Walls[1] = MakeWall(TEXT("FieldWall"), FIELD_ID, -2000.0, 0.0, 1000.0);
        Level.Portals[1].Location = FVector(-2500.0, 0.0, 0.0);
        Level.Arrivals[0].Location = FVector(-2500.0, 500.0, 0.0);
        ExpectRejected(TEXT("겹친 지역"), Level, MakeDesign(), { TEXT("TownVolume"), TEXT("FieldVolume"), TEXT("겹친다") });

        Level.Areas[1].Box = MakeBox(TEXT("FieldVolume"), -2500.0, 0.0, 1000.0, 1000.0);
        Level.Walls[1] = MakeWall(TEXT("FieldWall"), FIELD_ID, -2500.0, 0.0, 1000.0);
        TestEqual(TEXT("경계를 함께 쓰는 지역은 통과한다"), FP1PlacementCheck::Run({ Level }, MakeDesign()).GetExitCode(), 0);
    }

    // 3) 맵 경계 밖의 좌표.
    {
        FP1PlacementCheck::FLevel Level = MakeLevel();
        Level.Areas[1].Box = MakeBox(TEXT("FieldVolume"), -10000.0, 12000.0, 2500.0, 2500.0);
        Level.Walls[1] = MakeWall(TEXT("FieldWall"), FIELD_ID, -10000.0, 12000.0, 2500.0);
        ExpectRejected(TEXT("맵 경계 밖의 지역"), Level, MakeDesign(), { TEXT("FieldVolume"), TEXT("맵 경계 밖") });
    }

    // 4) 출발 지역 밖의 포털과 도착 지역 밖의 도착 지점.
    {
        FP1PlacementCheck::FLevel Level = MakeLevel();
        Level.Portals[0].Location = FVector(-3000.0, 0.0, 0.0);
        ExpectRejected(TEXT("지역 밖 포털"), Level, MakeDesign(), { TEXT("TownPortal"), TEXT("지역 10") });

        Level = MakeLevel();
        Level.Arrivals[0].Location = FVector(-1000.0, 0.0, 0.0);
        ExpectRejected(TEXT("도착 지역 밖의 도착 지점"), Level, MakeDesign(), { TEXT("FieldArrival"), TEXT("지역 20") });
    }

    // 5) 없는 기획 데이터 번호.
    {
        FP1PlacementCheck::FLevel Level = MakeLevel();
        Level.Areas[1].AreaId = 99;
        ExpectRejected(TEXT("없는 지역 번호"), Level, MakeDesign(), { TEXT("FieldVolume"), TEXT("99") });

        Level = MakeLevel();
        Level.Portals[1].Id = 99;
        ExpectRejected(TEXT("없는 포털 번호"), Level, MakeDesign(), { TEXT("FieldPortal"), TEXT("99") });

        Level = MakeLevel();
        Level.MapId = 9999;
        ExpectRejected(TEXT("없는 맵 번호"), Level, MakeDesign(), { TEXT("9999") });
    }

    // 6) 기획 데이터에 있는데 레벨에 없는 것.
    {
        FP1PlacementCheck::FLevel Level = MakeLevel();
        Level.Areas.RemoveAt(1);
        ExpectRejected(TEXT("지역 볼륨이 없는 지역"), Level, MakeDesign(), { TEXT("지역 20") });

        Level = MakeLevel();
        Level.Arrivals.RemoveAt(0);
        ExpectRejected(TEXT("도착 지점이 없는 포털"), Level, MakeDesign(), { TEXT("포털 11"), TEXT("도착 지점") });

        Level = MakeLevel();
        Level.MapMarkers.Empty();
        ExpectRejected(TEXT("맵 마커가 없는 레벨"), Level, MakeDesign(), { TEXT("L_Test"), TEXT("맵 마커") });
    }

    // 7) 리스폰 지점은 마을에 하나다.
    {
        FP1PlacementCheck::FLevel Level = MakeLevel();
        Level.RespawnPoints[0].Location = FVector(-10000.0, 10000.0, 0.0);
        ExpectRejected(TEXT("사냥터의 리스폰 지점"), Level, MakeDesign(), { TEXT("TownRespawn"), TEXT("지역 20") });

        Level = MakeLevel();
        Level.RespawnPoints.Empty();
        ExpectRejected(TEXT("리스폰 지점이 없는 마을"), Level, MakeDesign(), { TEXT("지역 10"), TEXT("리스폰 지점") });
    }

    // 8) 경계 벽은 지역 볼륨을 가리키고 그 크기와 같다. 볼륨을 옮기고 벽을 다시 맞추지 않으면 클라이언트가 서버와 다른 곳에서 막힌다.
    {
        FP1PlacementCheck::FLevel Level = MakeLevel();
        Level.Walls[1].AreaId = 0;
        ExpectRejected(TEXT("볼륨을 가리키지 않는 벽"), Level, MakeDesign(), { TEXT("FieldWall") });

        Level = MakeLevel();
        Level.Walls[1].HalfExtent = FVector2D(2000.0, 2500.0);
        ExpectRejected(TEXT("볼륨과 크기가 다른 벽"), Level, MakeDesign(), { TEXT("FieldWall"), TEXT("FieldVolume") });
    }

    // 9) 지역 볼륨과 맵 마커는 회전하지 않는다. 서버는 축에 맞춘 상자로만 판정한다.
    {
        FP1PlacementCheck::FLevel Level = MakeLevel();
        Level.Areas[0].Box.bRotated = true;
        ExpectRejected(TEXT("회전한 지역 볼륨"), Level, MakeDesign(), { TEXT("TownVolume"), TEXT("회전") });
    }

    return true;
}

#endif
