#pragma once

#include "CoreMinimal.h"

/**
 * 레벨에서 모은 배치 마커의 검사와 서버가 읽는 배치 데이터의 작성.
 *
 * 마커의 공간과 기획 데이터의 번호가 맞는지, 지역과 포털이 서로 맞게 놓였는지 검사하고, 통과하면 맵마다
 * 배치 데이터 JSON을 만든다. 레벨과 UObject를 모르므로 레벨을 연 쪽이 마커의 값을 넘긴다.
 */
class P1EDITOR_API FP1PlacementCheck
{
public:
    /** 축에 맞춘 상자. Center의 X, Y는 상자의 중심이고 Z는 바닥의 높이다. */
    struct FPlacedBox
    {
        /** 오류 메시지에 쓰는 액터 이름. */
        FString Name;
        FVector Center = FVector::ZeroVector;
        FVector2D HalfExtent = FVector2D::ZeroVector;
        /** 회전한 상자는 서버가 판정할 수 없다. */
        bool bRotated = false;
    };

    /** 맵 마커. 맵의 경계와 AOI 셀 크기(cm). */
    struct FPlacedMap
    {
        FPlacedBox Bounds;
        float CellSize = 0.f;
    };

    /** 지역 볼륨. */
    struct FPlacedArea
    {
        int32 AreaId = 0;
        FPlacedBox Box;
    };

    /** 위치와 방향만 있는 마커와 포털. Id는 포털 번호나 스폰 그룹 번호이고, Radius는 포털과 스폰 마커만 쓴다. */
    struct FPlacedPoint
    {
        FString Name;
        int32 Id = 0;
        FVector Location = FVector::ZeroVector;
        double Yaw = 0.0;
        double Radius = 0.0;
    };

    /** 경계 벽이 가리키는 지역 볼륨과 벽이 막는 안쪽 영역. AreaId가 0이면 볼륨을 가리키지 않는다. */
    struct FPlacedWall
    {
        FString Name;
        int32 AreaId = 0;
        FVector Center = FVector::ZeroVector;
        FVector2D HalfExtent = FVector2D::ZeroVector;
    };

    /** 맵 하나의 레벨에서 모은 마커. */
    struct FLevel
    {
        int32 MapId = 0;
        FString LevelName;
        TArray<FPlacedMap> MapMarkers;
        TArray<FPlacedArea> Areas;
        TArray<FPlacedPoint> RespawnPoints;
        TArray<FPlacedPoint> Portals;
        /** 포털 도착 지점. 다른 맵으로 가는 포털의 도착 지점은 도착 맵의 레벨에 있다. */
        TArray<FPlacedPoint> Arrivals;
        TArray<FPlacedWall> Walls;
        /** 몬스터 스폰 마커. Id가 스폰 그룹 번호다. */
        TArray<FPlacedPoint> SpawnPoints;
    };

    /** 검사가 대조하는 기획 데이터의 번호. */
    struct FDesignData
    {
        struct FArea
        {
            int32 MapId = 0;
            bool bHasRespawnPoint = false;
        };

        struct FPortal
        {
            /** 출발 지역. */
            int32 AreaId = 0;
            int32 DestAreaId = 0;
        };

        TSet<int32> MapIds;
        TMap<int32, FArea> Areas;
        TMap<int32, FPortal> Portals;
        TSet<int32> SpawnGroupIds;
    };

    struct FResult
    {
        /** 오류 한 줄에 액터 이름이나 번호가 들어 있다. */
        TArray<FString> Errors;
        /** 맵 번호 → 배치 데이터 JSON. 오류가 하나라도 있으면 비어 있다. */
        TMap<int32, FString> Files;

        int32 GetExitCode() const { return Errors.IsEmpty() ? 0 : 1; }
    };

    /**
     * 클라이언트 기획 데이터의 맵 표와 지역 표(포털이 중첩된), 서버 기획 데이터의 스폰 그룹 표 JSON에서 번호를 읽는다.
     * 형식이 틀리면 OutErrors에 사유를 더하고 false.
     */
    static bool ParseDesignData(const FString& MapJson, const FString& AreaJson, const FString& SpawnGroupJson,
        FDesignData& OutDesign, TArray<FString>& OutErrors);

    /** 추출할 모든 맵의 레벨을 함께 검사한다. 다른 맵으로 가는 포털의 도착 지점까지 보려고 레벨을 한 번에 받는다. */
    static FResult Run(const TArray<FLevel>& Levels, const FDesignData& Design);
};
