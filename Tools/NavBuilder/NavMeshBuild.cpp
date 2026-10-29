#include "NavMeshBuild.h"
#include <cmath>
#include <format>
#include <memory>
#include "Recast.h"
#include "DetourAlloc.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"

using namespace std;
using Json = nlohmann::json;

namespace
{
    // 복셀 크기(cm). 작을수록 벽과 경계가 정확하고 빌드가 느려진다. 바꾸면 NavMeshFile::FORMAT_VERSION을 올린다.
    constexpr float CELL_SIZE = 10.f;
    constexpr float CELL_HEIGHT = 5.f;
    // 폴리곤 한 변의 최대 길이(cm). 넓은 바닥이 긴 삼각형 몇 개로만 남으면 바닥 높이 보간이 거칠어진다.
    constexpr float MAX_EDGE_LENGTH = 1200.f;
    constexpr float MAX_SIMPLIFICATION_ERROR = 1.3f;
    constexpr int MIN_REGION_AREA = 8 * 8;
    constexpr int MERGE_REGION_AREA = 20 * 20;
    constexpr int MAX_VERTS_PER_POLY = 6;
    // 바닥 높이 질의가 쓰는 세부 메시의 표본 간격과 허용 오차(cm).
    constexpr float DETAIL_SAMPLE_DISTANCE = CELL_SIZE * 6.f;
    constexpr float DETAIL_SAMPLE_MAX_ERROR = CELL_HEIGHT;
    // 서버가 질의할 때 거르는 플래그. 걸을 수 있는 폴리곤은 모두 이 값이다.
    constexpr unsigned short WALKABLE_FLAG = 1;

    // Recast는 Y가 위인 좌표계다. 언리얼의 (x, y, z)를 (x, z, y)로 옮긴다.
    // 축을 맞바꾸면 좌표계가 뒤집혀 삼각형의 앞면도 뒤집히므로, 삼각형은 꼭짓점 순서를 거꾸로 넘긴다.
    // 그래야 바깥을 향하던 법선이 계속 바깥을 향하고, Recast가 위를 향한 면만 걸을 수 있는 면으로 고른다.
    vector<float> ToRecastVertices(const vector<float>& vertices)
    {
        vector<float> converted(vertices.size());
        for (size_t i = 0; i + 2 < vertices.size(); i += 3)
        {
            converted[i] = vertices[i];
            converted[i + 1] = vertices[i + 2];
            converted[i + 2] = vertices[i + 1];
        }
        return converted;
    }

    vector<int> ToRecastTriangles(const vector<int32_t>& triangles)
    {
        vector<int> converted(triangles.size());
        for (size_t i = 0; i + 2 < triangles.size(); i += 3)
        {
            converted[i] = triangles[i];
            converted[i + 1] = triangles[i + 2];
            converted[i + 2] = triangles[i + 1];
        }
        return converted;
    }

    double Round2(float value)
    {
        return round(static_cast<double>(value) * 100.0) / 100.0;
    }

    template<typename T, void (*Free)(T*)>
    struct RecastDeleter
    {
        void operator()(T* ptr) const { Free(ptr); }
    };

    using HeightfieldPtr = unique_ptr<rcHeightfield, RecastDeleter<rcHeightfield, rcFreeHeightField>>;
    using CompactHeightfieldPtr = unique_ptr<rcCompactHeightfield, RecastDeleter<rcCompactHeightfield, rcFreeCompactHeightfield>>;
    using ContourSetPtr = unique_ptr<rcContourSet, RecastDeleter<rcContourSet, rcFreeContourSet>>;
    using PolyMeshPtr = unique_ptr<rcPolyMesh, RecastDeleter<rcPolyMesh, rcFreePolyMesh>>;
    using PolyMeshDetailPtr = unique_ptr<rcPolyMeshDetail, RecastDeleter<rcPolyMeshDetail, rcFreePolyMeshDetail>>;
}

optional<string> NavMeshBuild::ParseGeometry(const Json& document, Geometry& outGeometry)
{
    if (document.is_object() == false)
        return string("형상 문서가 객체가 아니다");
    if (document.contains("MapId") == false || document["MapId"].is_number_integer() == false)
        return string("'MapId'가 없거나 정수가 아니다");
    if (document.contains("Vertices") == false || document["Vertices"].is_array() == false)
        return string("'Vertices'가 없거나 배열이 아니다");
    if (document.contains("Triangles") == false || document["Triangles"].is_array() == false)
        return string("'Triangles'가 없거나 배열이 아니다");

    Geometry geometry;
    geometry.mapId = document["MapId"].get<int32_t>();

    const Json& vertices = document["Vertices"];
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        const Json& vertex = vertices[i];
        if (vertex.is_array() == false || vertex.size() != 3 || vertex[0].is_number() == false || vertex[1].is_number() == false || vertex[2].is_number() == false)
            return format("{}번째 꼭짓점이 수 세 개가 아니다", i);

        for (int axis = 0; axis < 3; ++axis)
            geometry.vertices.push_back(vertex[axis].get<float>());
    }

    const int32_t vertexCount = static_cast<int32_t>(vertices.size());
    const Json& triangles = document["Triangles"];
    for (size_t i = 0; i < triangles.size(); ++i)
    {
        const Json& triangle = triangles[i];
        if (triangle.is_array() == false || triangle.size() != 3)
            return format("{}번째 삼각형이 꼭짓점 번호 세 개가 아니다", i);

        for (int corner = 0; corner < 3; ++corner)
        {
            if (triangle[corner].is_number_integer() == false)
                return format("{}번째 삼각형의 꼭짓점 번호가 정수가 아니다", i);

            const int32_t index = triangle[corner].get<int32_t>();
            if (index < 0 || index >= vertexCount)
                return format("{}번째 삼각형이 없는 꼭짓점 {}을 가리킨다", i, index);

            geometry.triangles.push_back(index);
        }
    }

    if (geometry.triangles.empty())
        return string("삼각형이 없다");

    outGeometry = std::move(geometry);
    return nullopt;
}

optional<string> NavMeshBuild::Build(const Geometry& geometry, const NavMeshFile::AgentSettings& agent, vector<uint8_t>& outFile)
{
    const vector<float> vertices = ToRecastVertices(geometry.vertices);
    const vector<int> triangles = ToRecastTriangles(geometry.triangles);
    const int vertexCount = static_cast<int>(vertices.size() / 3);
    const int triangleCount = static_cast<int>(triangles.size() / 3);
    if (triangleCount == 0)
        return string("삼각형이 없다");

    rcConfig config{};
    config.cs = CELL_SIZE;
    config.ch = CELL_HEIGHT;
    config.walkableSlopeAngle = agent.maxSlope;
    config.walkableHeight = static_cast<int>(ceilf(agent.height / CELL_HEIGHT));
    config.walkableClimb = static_cast<int>(floorf(agent.maxStep / CELL_HEIGHT));
    config.walkableRadius = static_cast<int>(ceilf(agent.radius / CELL_SIZE));
    config.maxEdgeLen = static_cast<int>(MAX_EDGE_LENGTH / CELL_SIZE);
    config.maxSimplificationError = MAX_SIMPLIFICATION_ERROR;
    config.minRegionArea = MIN_REGION_AREA;
    config.mergeRegionArea = MERGE_REGION_AREA;
    config.maxVertsPerPoly = MAX_VERTS_PER_POLY;
    config.detailSampleDist = DETAIL_SAMPLE_DISTANCE;
    config.detailSampleMaxError = DETAIL_SAMPLE_MAX_ERROR;
    rcCalcBounds(vertices.data(), vertexCount, config.bmin, config.bmax);
    rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

    // 로그와 시간 측정을 끈다. 켜도 결과는 같지만 출력이 실행마다 달라진다.
    rcContext context(false);

    // 삼각형을 복셀로 쌓고, 에이전트가 설 수 없는 칸을 거른다.
    HeightfieldPtr solid(rcAllocHeightfield());
    if (solid == nullptr || rcCreateHeightfield(&context, *solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch) == false)
        return format("높이 필드({} x {})를 만들지 못했다", config.width, config.height);

    vector<unsigned char> triangleAreas(triangleCount, 0);
    rcMarkWalkableTriangles(&context, config.walkableSlopeAngle, vertices.data(), vertexCount, triangles.data(), triangleCount, triangleAreas.data());
    if (rcRasterizeTriangles(&context, vertices.data(), vertexCount, triangles.data(), triangleAreas.data(), triangleCount, *solid, config.walkableClimb) == false)
        return string("삼각형을 복셀로 옮기지 못했다");

    rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, *solid);
    rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, *solid);
    rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, *solid);

    // 걸을 수 있는 칸을 에이전트 반경만큼 깎고 영역으로 나눈다.
    CompactHeightfieldPtr compact(rcAllocCompactHeightfield());
    if (compact == nullptr || rcBuildCompactHeightfield(&context, config.walkableHeight, config.walkableClimb, *solid, *compact) == false)
        return string("압축 높이 필드를 만들지 못했다");
    solid.reset();

    if (rcErodeWalkableArea(&context, config.walkableRadius, *compact) == false)
        return string("에이전트 반경만큼 깎지 못했다");
    if (rcBuildDistanceField(&context, *compact) == false)
        return string("거리 필드를 만들지 못했다");
    if (rcBuildRegions(&context, *compact, 0, config.minRegionArea, config.mergeRegionArea) == false)
        return string("영역을 나누지 못했다");

    // 영역의 윤곽에서 폴리곤 메시와 높이 세부 메시를 만든다.
    ContourSetPtr contours(rcAllocContourSet());
    if (contours == nullptr || rcBuildContours(&context, *compact, config.maxSimplificationError, config.maxEdgeLen, *contours) == false)
        return string("윤곽을 만들지 못했다");

    PolyMeshPtr polyMesh(rcAllocPolyMesh());
    if (polyMesh == nullptr || rcBuildPolyMesh(&context, *contours, config.maxVertsPerPoly, *polyMesh) == false)
        return string("폴리곤 메시를 만들지 못했다");
    if (polyMesh->npolys == 0)
        return string("걸을 수 있는 면이 없다. 바닥의 충돌과 에이전트 설정을 확인한다");
    if (polyMesh->nverts >= 0xffff)
        return format("꼭짓점이 {}개라 타일 하나에 담지 못한다", polyMesh->nverts);

    PolyMeshDetailPtr detailMesh(rcAllocPolyMeshDetail());
    if (detailMesh == nullptr || rcBuildPolyMeshDetail(&context, *polyMesh, *compact, config.detailSampleDist, config.detailSampleMaxError, *detailMesh) == false)
        return string("높이 세부 메시를 만들지 못했다");

    for (int i = 0; i < polyMesh->npolys; ++i)
        polyMesh->flags[i] = polyMesh->areas[i] == RC_WALKABLE_AREA ? WALKABLE_FLAG : 0;

    // 맵 전체를 타일 하나로 담는다.
    dtNavMeshCreateParams params{};
    params.verts = polyMesh->verts;
    params.vertCount = polyMesh->nverts;
    params.polys = polyMesh->polys;
    params.polyAreas = polyMesh->areas;
    params.polyFlags = polyMesh->flags;
    params.polyCount = polyMesh->npolys;
    params.nvp = polyMesh->nvp;
    params.detailMeshes = detailMesh->meshes;
    params.detailVerts = detailMesh->verts;
    params.detailVertsCount = detailMesh->nverts;
    params.detailTris = detailMesh->tris;
    params.detailTriCount = detailMesh->ntris;
    params.walkableHeight = agent.height;
    params.walkableRadius = agent.radius;
    params.walkableClimb = agent.maxStep;
    rcVcopy(params.bmin, polyMesh->bmin);
    rcVcopy(params.bmax, polyMesh->bmax);
    params.cs = config.cs;
    params.ch = config.ch;
    params.buildBvTree = true;

    unsigned char* tileData = nullptr;
    int tileDataSize = 0;
    if (dtCreateNavMeshData(&params, &tileData, &tileDataSize) == false)
        return string("Detour 타일 데이터를 만들지 못했다");

    outFile = NavMeshFile::Write(agent, tileData, static_cast<uint32_t>(tileDataSize));
    dtFree(tileData);
    return nullopt;
}

optional<string> NavMeshBuild::DumpPolygons(const vector<uint8_t>& file, int32_t mapId, const NavMeshFile::AgentSettings& agent, Json& outDocument)
{
    NavMeshFile::TileData tile;
    if (optional<string> error = NavMeshFile::Read(file, NavMeshFile::HashAgent(agent), tile))
        return error;

    // Detour가 타일 데이터를 갖고 해제하므로 Detour의 할당자로 복사해 넘긴다.
    unsigned char* data = static_cast<unsigned char*>(dtAlloc(tile.size, DT_ALLOC_PERM));
    memcpy(data, tile.data, tile.size);
    dtNavMesh navMesh;
    if (dtStatusFailed(navMesh.init(data, static_cast<int>(tile.size), DT_TILE_FREE_DATA)))
    {
        dtFree(data);
        return string("Detour가 타일 데이터를 읽지 못했다");
    }

    Json polygons = Json::array();
    const dtNavMesh& constNavMesh = navMesh;
    for (int tileIndex = 0; tileIndex < constNavMesh.getMaxTiles(); ++tileIndex)
    {
        const dtMeshTile* meshTile = constNavMesh.getTile(tileIndex);
        if (meshTile == nullptr || meshTile->header == nullptr)
            continue;

        for (int polyIndex = 0; polyIndex < meshTile->header->polyCount; ++polyIndex)
        {
            const dtPoly& poly = meshTile->polys[polyIndex];
            if (poly.getType() != DT_POLYTYPE_GROUND)
                continue;

            Json polygon = Json::array();
            for (int corner = 0; corner < poly.vertCount; ++corner)
            {
                const float* vertex = &meshTile->verts[poly.verts[corner] * 3];
                polygon.push_back({ Round2(vertex[0]), Round2(vertex[2]), Round2(vertex[1]) });
            }
            polygons.push_back(std::move(polygon));
        }
    }

    outDocument = Json{ { "MapId", mapId }, { "Polygons", std::move(polygons) } };
    return nullopt;
}
