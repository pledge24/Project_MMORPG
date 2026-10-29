#include "Placement/P1PlacementExtractCommandlet.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/World/P1AreaVolume.h"
#include "Game/World/P1FieldBoundaryWall.h"
#include "Game/World/P1MapMarker.h"
#include "Game/World/P1MonsterSpawnMarker.h"
#include "Game/World/P1Portal.h"
#include "Game/World/P1PortalArrivalMarker.h"
#include "Game/World/P1RespawnPointMarker.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Navigation/P1NavGeometry.h"
#include "Placement/P1PlacementCheck.h"
#include "UObject/Package.h"
#include "Utils/LogCategory.h"

namespace
{
    /** 서버가 읽는 추출 산출물 폴더. 서버의 Gamedata가 읽는 폴더(Game/Data/Extracted/)와 같아야 한다. */
    FString GetOutputDir()
    {
        return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../Server/GameServer/Game/Data/Extracted/"));
    }

    /**
     * 서버가 읽는 DataBuilder 산출물 폴더. databuilder.toml의 game 모듈 폴더와 같아야 한다.
     * 스폰 그룹 표는 클라이언트로 가는 열이 없어 클라이언트 산출물이 없으므로, 마커가 가리키는 번호를 여기서 읽는다.
     */
    FString GetServerDesignDataDir()
    {
        return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../Server/GameServer/Game/Data/Generated/"));
    }

    /** 서버의 PLACEMENT_FILE_PREFIX와 같다. */
    const TCHAR* PLACEMENT_FILE_PREFIX = TEXT("Placement_");
    /** 서버와 빌드 도구의 NavMeshFile::AGENT_FILE과 같다. */
    const TCHAR* NAV_AGENT_FILE = TEXT("NavAgent.json");
    /** 빌드 도구(Tools/NavBuilder)가 읽는 형상 파일의 접두사. */
    const TCHAR* NAV_GEOMETRY_FILE_PREFIX = TEXT("NavGeometry_");

    /** navmesh 빌드 도구에 넘기는 형상 폴더. 중간 산출물이라 추적하지 않는다. 추출 스크립트가 같은 폴더를 도구에 넘긴다. */
    FString GetNavGeometryDir()
    {
        return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("NavExtract/"));
    }

    bool IsRotated(const AActor& Actor)
    {
        return Actor.GetActorRotation().IsNearlyZero(0.01) == false;
    }

    FP1PlacementCheck::FPlacedBox MakeBox(const AP1PlacementVolume& Volume)
    {
        FP1PlacementCheck::FPlacedBox Box;
        Box.Name = Volume.GetActorNameOrLabel();
        Box.Center = Volume.GetActorLocation();
        Box.HalfExtent = Volume.GetHalfExtent();
        Box.bRotated = IsRotated(Volume);
        return Box;
    }

    FP1PlacementCheck::FPlacedPoint MakePoint(const AActor& Actor, int32 Id)
    {
        FP1PlacementCheck::FPlacedPoint Point;
        Point.Name = Actor.GetActorNameOrLabel();
        Point.Id = Id;
        Point.Location = Actor.GetActorLocation();
        Point.Yaw = Actor.GetActorRotation().Yaw;
        return Point;
    }

    /** 레벨의 액터에서 마커를 모은다. 컴포넌트를 등록한 월드여야 붙은 액터의 위치가 맞다. */
    FP1PlacementCheck::FLevel CollectLevel(int32 MapId, const UWorld& World)
    {
        FP1PlacementCheck::FLevel Level;
        Level.MapId = MapId;
        Level.LevelName = World.GetName();

        for (AActor* Actor : World.PersistentLevel->Actors)
        {
            if (Actor == nullptr)
                continue;

            if (const AP1MapMarker* MapMarker = Cast<AP1MapMarker>(Actor))
                Level.MapMarkers.Add({ MakeBox(*MapMarker), MapMarker->GetCellSize() });
            else if (const AP1AreaVolume* Area = Cast<AP1AreaVolume>(Actor))
                Level.Areas.Add({ Area->GetAreaId(), MakeBox(*Area) });
            else if (Cast<AP1RespawnPointMarker>(Actor) != nullptr)
                Level.RespawnPoints.Add(MakePoint(*Actor, 0));
            else if (const AP1PortalArrivalMarker* Arrival = Cast<AP1PortalArrivalMarker>(Actor))
                Level.Arrivals.Add(MakePoint(*Arrival, Arrival->GetPortalId()));
            else if (const AP1MonsterSpawnMarker* Spawn = Cast<AP1MonsterSpawnMarker>(Actor))
            {
                FP1PlacementCheck::FPlacedPoint Point = MakePoint(*Spawn, Spawn->GetSpawnGroupId());
                Point.Radius = Spawn->GetRadius();
                Level.SpawnPoints.Add(Point);
            }
            else if (const AP1Portal* Portal = Cast<AP1Portal>(Actor))
            {
                FP1PlacementCheck::FPlacedPoint Point = MakePoint(*Portal, Portal->GetPortalId());
                Point.Radius = Portal->GetUseRadius();
                Level.Portals.Add(Point);
            }
            else if (const AP1FieldBoundaryWall* Wall = Cast<AP1FieldBoundaryWall>(Actor))
            {
                FP1PlacementCheck::FPlacedWall Placed;
                Placed.Name = Wall->GetActorNameOrLabel();
                Placed.AreaId = Wall->GetAreaVolume() != nullptr ? Wall->GetAreaVolume()->GetAreaId() : 0;
                Placed.Center = Wall->GetFittedCenter();
                Placed.HalfExtent = Wall->GetFittedHalfExtent();
                Level.Walls.Add(Placed);
            }
        }

        return Level;
    }

    /** 맵 레벨을 열어 마커와 충돌 형상을 모은다. 레벨을 열지 못하면 OutErrors에 더하고 false. 형상의 오류도 OutErrors에 더한다. */
    bool LoadLevel(int32 MapId, const TSoftObjectPtr<UWorld>& LevelRef, FP1PlacementCheck::FLevel& OutLevel, FP1NavGeometry& OutGeometry, TArray<FString>& OutErrors)
    {
        const FString PackageName = LevelRef.ToSoftObjectPath().GetLongPackageName();
        UPackage* Package = LoadPackage(nullptr, *PackageName, LOAD_None);
        UWorld* World = Package != nullptr ? UWorld::FindWorldInPackage(Package) : nullptr;
        if (World == nullptr)
        {
            OutErrors.Add(FString::Printf(TEXT("맵 %d의 레벨 %s를 열지 못했다"), MapId, *PackageName));
            return false;
        }

        // 다른 액터에 붙은 액터(바닥에 붙은 포털)의 월드 위치는 컴포넌트를 등록해야 계산된다.
        World->AddToRoot();
        const bool bInitializedHere = World->bIsWorldInitialized == false;
        if (bInitializedHere)
        {
            World->WorldType = EWorldType::Editor;
            World->InitWorld(UWorld::InitializationValues()
                .AllowAudioPlayback(false)
                .CreatePhysicsScene(false)
                .RequiresHitProxies(false)
                .CreateNavigation(false)
                .CreateAISystem(false)
                .ShouldSimulatePhysics(false)
                .SetTransactional(false));
        }
        World->UpdateWorldComponents(true, false);

        OutLevel = CollectLevel(MapId, *World);
        OutGeometry = FP1NavGeometry::CollectLevel(*World, OutErrors);
        // 스폰 마커가 걸을 수 있는 바닥 위에 있는지는 navmesh를 빌드한 뒤에야 알 수 있어 빌드 도구가 검사한다.
        for (const FP1PlacementCheck::FPlacedPoint& Spawn : OutLevel.SpawnPoints)
            OutGeometry.SpawnPoints.Add({ Spawn.Name, Spawn.Location });

        if (bInitializedHere)
            World->CleanupWorld();
        World->RemoveFromRoot();
        return true;
    }

    /** 산출물을 쓰고 이번에 쓰지 않은 맵의 옛 산출물을 지운다. 사람이 쓴 파일이 없는 폴더라서 접두사로 가린다. */
    bool WriteFiles(const TMap<int32, FString>& Files)
    {
        const FString OutputDir = GetOutputDir();
        IFileManager::Get().MakeDirectory(*OutputDir, true);

        TArray<FString> Existing;
        IFileManager::Get().FindFiles(Existing, *(OutputDir / FString(PLACEMENT_FILE_PREFIX) + TEXT("*.json")), true, false);

        TSet<FString> Written;
        for (const TPair<int32, FString>& File : Files)
        {
            const FString FileName = FString::Printf(TEXT("%s%d.json"), PLACEMENT_FILE_PREFIX, File.Key);
            if (FFileHelper::SaveStringToFile(File.Value, *(OutputDir / FileName), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) == false)
            {
                UE_LOG(LogP1Placement, Error, TEXT("[배치 추출] %s를 쓰지 못했다"), *(OutputDir / FileName));
                return false;
            }
            Written.Add(FileName);
            UE_LOG(LogP1Placement, Display, TEXT("[배치 추출] %s"), *(OutputDir / FileName));
        }

        for (const FString& FileName : Existing)
        {
            if (Written.Contains(FileName) == false)
            {
                IFileManager::Get().Delete(*(OutputDir / FileName));
                UE_LOG(LogP1Placement, Display, TEXT("[배치 추출] 맵 표에 없는 맵의 옛 산출물 %s를 지웠다"), *FileName);
            }
        }

        return true;
    }

    /**
     * 빌드 도구가 읽는 형상과 서버가 읽는 에이전트 설정을 쓴다. 형상 폴더의 옛 형상은 먼저 지운다.
     * 지우지 않으면 맵 표에서 빠진 맵의 navmesh를 도구가 계속 빌드한다.
     */
    bool WriteNavFiles(const TMap<int32, FP1NavGeometry>& Geometries, const TMap<int32, TSoftObjectPtr<UWorld>>& MapLevels, const FP1NavGeometry::FAgent& Agent)
    {
        const FString GeometryDir = GetNavGeometryDir();
        IFileManager::Get().MakeDirectory(*GeometryDir, true);

        TArray<FString> Existing;
        IFileManager::Get().FindFiles(Existing, *(GeometryDir / FString(NAV_GEOMETRY_FILE_PREFIX) + TEXT("*.json")), true, false);
        for (const FString& FileName : Existing)
            IFileManager::Get().Delete(*(GeometryDir / FileName));

        for (const TPair<int32, FP1NavGeometry>& Geometry : Geometries)
        {
            const FString Path = GeometryDir / FString::Printf(TEXT("%s%d.json"), NAV_GEOMETRY_FILE_PREFIX, Geometry.Key);
            const FString LevelPath = MapLevels.FindChecked(Geometry.Key).ToSoftObjectPath().GetLongPackageName();
            if (FFileHelper::SaveStringToFile(Geometry.Value.ToJson(Geometry.Key, LevelPath), *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) == false)
            {
                UE_LOG(LogP1Placement, Error, TEXT("[배치 추출] %s를 쓰지 못했다"), *Path);
                return false;
            }
            UE_LOG(LogP1Placement, Display, TEXT("[배치 추출] %s (삼각형 %d개)"), *Path, Geometry.Value.Triangles.Num() / 3);
        }

        const FString AgentPath = GetOutputDir() / NAV_AGENT_FILE;
        if (FFileHelper::SaveStringToFile(FP1NavGeometry::AgentToJson(Agent), *AgentPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) == false)
        {
            UE_LOG(LogP1Placement, Error, TEXT("[배치 추출] %s를 쓰지 못했다"), *AgentPath);
            return false;
        }
        UE_LOG(LogP1Placement, Display, TEXT("[배치 추출] %s"), *AgentPath);
        return true;
    }
}

UP1PlacementExtractCommandlet::UP1PlacementExtractCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UP1PlacementExtractCommandlet::Main(const FString& Params)
{
    TArray<FString> Errors;

    // 기획 데이터의 번호. 레벨의 마커가 이 번호를 가리킨다.
    const FString DesignDir = FPaths::ProjectDir() / TEXT("DesignData/Generated/");
    FString MapJson;
    FString AreaJson;
    FString SpawnGroupJson;
    FP1PlacementCheck::FDesignData Design;
    if (FFileHelper::LoadFileToString(MapJson, *(DesignDir / TEXT("Map.json"))) == false
        || FFileHelper::LoadFileToString(AreaJson, *(DesignDir / TEXT("Area.json"))) == false)
        Errors.Add(FString::Printf(TEXT("%s에서 Map.json이나 Area.json을 읽지 못했다. DataBuilder를 돌린다"), *DesignDir));
    else if (FFileHelper::LoadFileToString(SpawnGroupJson, *(GetServerDesignDataDir() / TEXT("SpawnGroup.json"))) == false)
        Errors.Add(FString::Printf(TEXT("%s에서 SpawnGroup.json을 읽지 못했다. DataBuilder를 돌린다"), *GetServerDesignDataDir()));
    else
        FP1PlacementCheck::ParseDesignData(MapJson, AreaJson, SpawnGroupJson, Design, Errors);

    // navmesh의 에이전트는 플레이어 캡슐과 이동 컴포넌트다.
    FP1NavGeometry::FAgent Agent;
    if (Errors.IsEmpty())
        FP1NavGeometry::ReadAgent(*GetDefault<UP1GameDataSettings>(), Agent, Errors);

    // 맵 표의 레벨을 모두 연다. 맵 번호 순서로 열어 오류의 순서가 실행마다 같다.
    TArray<FP1PlacementCheck::FLevel> Levels;
    TMap<int32, FP1NavGeometry> Geometries;
    if (Errors.IsEmpty())
    {
        TMap<int32, TSoftObjectPtr<UWorld>> MapLevels = GetDefault<UP1GameDataSettings>()->MapLevels;
        MapLevels.KeySort(TLess<int32>());
        for (const TPair<int32, TSoftObjectPtr<UWorld>>& MapLevel : MapLevels)
        {
            FP1PlacementCheck::FLevel Level;
            FP1NavGeometry Geometry;
            if (LoadLevel(MapLevel.Key, MapLevel.Value, Level, Geometry, Errors))
            {
                Levels.Add(MoveTemp(Level));
                Geometries.Add(MapLevel.Key, MoveTemp(Geometry));
            }
        }
    }

    FP1PlacementCheck::FResult Result;
    if (Errors.IsEmpty())
    {
        Result = FP1PlacementCheck::Run(Levels, Design);
        Errors = Result.Errors;
    }

    if (Errors.IsEmpty() == false)
    {
        for (const FString& Error : Errors)
            UE_LOG(LogP1Placement, Error, TEXT("[배치 추출] %s"), *Error);
        UE_LOG(LogP1Placement, Error, TEXT("[배치 추출] 오류 %d건. 산출물을 바꾸지 않았다"), Errors.Num());
        return 1;
    }

    if (WriteFiles(Result.Files) == false || WriteNavFiles(Geometries, GetDefault<UP1GameDataSettings>()->MapLevels, Agent) == false)
        return 1;

    UE_LOG(LogP1Placement, Display, TEXT("[배치 추출] 맵 %d개의 배치 데이터를 썼다"), Result.Files.Num());
    return 0;
}
