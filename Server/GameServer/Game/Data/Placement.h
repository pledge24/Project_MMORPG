#pragma once
#include "Protocol/Entity.pb.h"
#include "Utils/PlaneBounds.h"
#include "Utils/Utils.h"

/*--------------------------------------------------------------
    배치 데이터

    에디터 레벨에 놓은 마커에서 추출한 공간 정보다. 기획 데이터 표와 행 번호로 이어진다.
    규칙과 수치는 기획 데이터 표에 있고, 여기에는 위치와 크기만 있다.
---------------------------------------------------------------*/

/** 맵 하나의 배치 데이터 파일 이름은 이 접두사 뒤에 맵 번호를 붙인다(Placement_1111.json). */
constexpr string_view PLACEMENT_FILE_PREFIX = "Placement_";

/** 맵 마커에서 추출한 맵의 경계와 AOI 셀 크기. */
struct MapPlacement
{
    int32 mapId = 0;
    PlaneBounds bounds;
    /** cm. 0보다 크다. */
    float cellSize = 0.f;
};

/** 지역 볼륨에서 추출한 지역의 공간. center의 z는 지역 바닥의 높이다. */
struct AreaPlacement
{
    int32 id = 0;
    vector3D center;
    PlaneBounds bounds;
};

/** 리스폰 지점 마커. 소속 지역은 추출할 때 포함 판정으로 정했다. */
struct RespawnPointPlacement
{
    int32 areaId = 0;
    vector3D pos;
    float yaw = 0.f;
};

/** 레벨의 포털과 그 도착 지점 마커. src와 dest의 z는 바닥의 높이다. */
struct PortalPlacement
{
    int32 id = 0;
    vector3D src;
    /** 플레이어가 포털을 쓸 수 있는 평면 거리(cm). */
    float radius = 0.f;
    vector3D dest;
    float destYaw = 0.f;
};
