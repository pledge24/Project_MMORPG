#pragma once

#include "CoreMinimal.h"

class UP1GameDataSettings;
class UWorld;

/**
 * navmesh 빌드 도구(Tools/NavBuilder)에 넘기는 맵 하나의 충돌 형상과 에이전트 설정.
 *
 * 레벨에서 폰을 막는 충돌을 삼각형으로 모으고, 플레이어 블루프린트에서 에이전트 설정을 읽어 도구가 읽는 JSON으로 쓴다.
 * 삼각형은 언리얼 좌표에서 오른손 법칙으로 형상의 바깥을 향한다.
 */
struct FP1NavGeometry
{
    /** 플레이어 캡슐과 이동 컴포넌트의 값. 길이는 cm, 경사는 도다. */
    struct FAgent
    {
        double Radius = 0.0;
        double Height = 0.0;
        double MaxSlope = 0.0;
        double MaxStep = 0.0;
    };

    TArray<FVector> Vertices;
    /** 꼭짓점 번호 셋이 삼각형 하나다. */
    TArray<int32> Triangles;
    /** 빌드한 navmesh 위에 있어야 하는 몬스터 스폰 마커. 액터 이름과 위치(바닥)다. 빌드 도구가 검사한다. */
    TArray<TPair<FString, FVector>> SpawnPoints;

    /**
     * 레벨의 액터에서 폰을 막는 충돌을 모은다. 폰은 뺀다. 컴포넌트를 등록한 월드여야 위치가 맞다.
     * 상자와 볼록 형상만 지원하고, 그 밖의 형상(구, 캡슐, 복잡한 충돌)을 만나면 OutErrors에 더한다.
     */
    static FP1NavGeometry CollectLevel(const UWorld& World, TArray<FString>& OutErrors);

    /**
     * 직업마다의 플레이어 블루프린트(CharacterClassPlayers)에서 에이전트 설정을 읽는다.
     * 블루프린트를 열 수 없거나 직업마다 값이 다르면(navmesh 하나로 덮지 못한다) OutErrors에 더하고 false.
     */
    static bool ReadAgent(const UP1GameDataSettings& Settings, FAgent& OutAgent, TArray<FString>& OutErrors);

    /**
     * 빌드 도구가 읽는 형상 문서(NavGeometry_<맵 번호>.json). 좌표는 소수점 둘째 자리에서 반올림한다.
     * LevelPath는 레벨의 패키지 경로이고, 에디터 메뉴가 겹쳐 그릴 레벨을 찾는다.
     */
    FString ToJson(int32 MapId, const FString& LevelPath) const;
    /** 서버와 빌드 도구가 읽는 에이전트 설정 문서(NavAgent.json). */
    static FString AgentToJson(const FAgent& Agent);
};
