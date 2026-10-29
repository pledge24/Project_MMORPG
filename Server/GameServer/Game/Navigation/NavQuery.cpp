#include "Core/pch.h"
#include "Game/Navigation/NavQuery.h"
#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

NavQuery::NavQuery(shared_ptr<const NavMesh> navMesh)
    : _navMesh(std::move(navMesh))
    , _filter(make_unique<dtQueryFilter>())
{
    _query = dtAllocNavMeshQuery();
    if (_query == nullptr || dtStatusFailed(_query->init(&_navMesh->GetDetour(), MAX_SEARCH_NODES)))
        GLogger->Error("navmesh 질의 객체를 만들지 못했다");
}

NavQuery::~NavQuery()
{
    dtFreeNavMeshQuery(_query);
}

vector<vector3D> NavQuery::FindPath(const vector3D& from, const vector3D& to)
{
    float startPos[3];
    float endPos[3];
    const dtPolyRef startRef = FindNearestPoly(from, OUT startPos);
    const dtPolyRef endRef = FindNearestPoly(to, OUT endPos);
    if (startRef == 0 || endRef == 0)
        return {};

    dtPolyRef polys[MAX_PATH_POLYS];
    int polyCount = 0;
    const dtStatus status = _query->findPath(startRef, endRef, startPos, endPos, _filter.get(), polys, &polyCount, MAX_PATH_POLYS);
    if (dtStatusFailed(status) || polyCount == 0)
        return {};

    // 끝에 닿지 못한 경로는 마지막 폴리곤에서 끝에 가장 가까운 점까지 간다.
    if (polys[polyCount - 1] != endRef)
        _query->closestPointOnPoly(polys[polyCount - 1], endPos, endPos, nullptr);

    float corners[MAX_PATH_POLYS * 3];
    int cornerCount = 0;
    if (dtStatusFailed(_query->findStraightPath(startPos, endPos, polys, polyCount, corners, nullptr, nullptr, &cornerCount, MAX_PATH_POLYS)))
        return {};

    vector<vector3D> path;
    path.reserve(cornerCount);
    for (int i = 0; i < cornerCount; ++i)
        path.push_back(FromDetour(&corners[i * 3]));
    return path;
}

optional<vector3D> NavQuery::MoveAlongSurface(const vector3D& from, const vector3D& to)
{
    float startPos[3];
    const dtPolyRef startRef = FindNearestPoly(from, OUT startPos);
    if (startRef == 0)
        return nullopt;

    float endPos[3];
    ToDetour(to, OUT endPos);

    float resultPos[3];
    dtPolyRef visited[MAX_SURFACE_VISITS];
    int visitedCount = 0;
    if (dtStatusFailed(_query->moveAlongSurface(startRef, startPos, endPos, _filter.get(), resultPos, visited, &visitedCount, MAX_SURFACE_VISITS))
        || visitedCount == 0)
        return nullopt;

    // 결과의 높이는 출발점의 높이 그대로라서, 멈춘 폴리곤의 표면 높이로 맞춘다.
    float height = resultPos[1];
    if (dtStatusSucceed(_query->getPolyHeight(visited[visitedCount - 1], resultPos, &height)))
        resultPos[1] = height;

    return FromDetour(resultPos);
}

optional<vector3D> NavQuery::Raycast(const vector3D& from, const vector3D& to)
{
    float startPos[3];
    const dtPolyRef startRef = FindNearestPoly(from, OUT startPos);
    if (startRef == 0)
        return from;

    float endPos[3];
    ToDetour(to, OUT endPos);

    dtRaycastHit hit{};
    if (dtStatusFailed(_query->raycast(startRef, startPos, endPos, _filter.get(), 0, &hit)))
        return from;

    // 끝까지 막히지 않으면 t가 FLT_MAX다.
    if (hit.t > 1.f)
        return nullopt;

    return vector3D(
        from.x + (to.x - from.x) * hit.t,
        from.y + (to.y - from.y) * hit.t,
        from.z + (to.z - from.z) * hit.t);
}

optional<float> NavQuery::FindFloorHeight(const vector3D& pos)
{
    float nearestPoint[3];
    if (FindNearestPoly(pos, OUT nearestPoint) == 0)
        return nullopt;

    return nearestPoint[1];
}

uint32 NavQuery::FindNearestPoly(const vector3D& pos, OUT float* nearestPoint)
{
    // 벽에 붙어 선 에이전트의 중심은 반경만큼 깎인 navmesh의 가장자리를 조금 벗어날 수 있어, 평면으로 반경만큼 찾는다.
    // 높이는 캡슐 하나만큼 위아래를 찾는다. 그보다 멀리 있는 면은 그 위치의 바닥이 아니다.
    const NavMeshFile::AgentSettings& agent = _navMesh->GetAgent();
    const float halfExtents[3] = { agent.radius, agent.height, agent.radius };

    float center[3];
    ToDetour(pos, OUT center);

    dtPolyRef ref = 0;
    if (_query == nullptr || dtStatusFailed(_query->findNearestPoly(center, halfExtents, _filter.get(), &ref, nearestPoint)))
        return 0;

    return ref;
}

void NavQuery::ToDetour(const vector3D& pos, OUT float* detourPos) const
{
    // Detour는 y가 위다. 빌드 도구(NavMeshBuild)와 같은 축 바꾸기다.
    detourPos[0] = pos.x;
    detourPos[1] = pos.z - GetAgentHalfHeight();
    detourPos[2] = pos.y;
}

vector3D NavQuery::FromDetour(const float* detourPos) const
{
    return vector3D(detourPos[0], detourPos[2], detourPos[1] + GetAgentHalfHeight());
}
