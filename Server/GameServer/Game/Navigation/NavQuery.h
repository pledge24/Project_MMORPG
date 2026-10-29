#pragma once
#include "Game/Navigation/NavMesh.h"
#include "Utils/Utils.h"

class dtNavMeshQuery;
class dtQueryFilter;

/**
 * 룸 하나가 맵의 지형에 묻는 창구.
 *
 * 맵의 navmesh에 길찾기, 표면을 따른 이동, 직선 통과, 바닥 높이를 묻는다. 질의 상태를 가지므로 룸마다 하나를 두고 룸 큐 위에서만 쓴다.
 * 좌표는 언리얼 단위(cm, z가 위)다. 위치의 z는 바닥이 아니라 에이전트 캡슐의 중심이다.
 */
class NavQuery
{
public:
    explicit NavQuery(shared_ptr<const NavMesh> navMesh);
    ~NavQuery();
    NavQuery(const NavQuery&) = delete;
    NavQuery& operator=(const NavQuery&) = delete;

    /**
     * from에서 to까지 꺾이는 점들. 첫 점은 from이 놓인 navmesh 위의 점이고 마지막 점은 to이거나, to에 닿지 못하면 가장 가까이 가는 점이다.
     * from이나 to 근처에 걸을 수 있는 면이 없으면 빈 목록.
     */
    vector<vector3D> FindPath(const vector3D& from, const vector3D& to);
    /** 표면을 따라 from에서 to로 간 끝. 벽에 닿으면 벽을 따라 미끄러지다 멈춘다. from 근처에 걸을 수 있는 면이 없으면 nullopt. */
    optional<vector3D> MoveAlongSurface(const vector3D& from, const vector3D& to);
    /**
     * from에서 to까지의 직선이 벽에 막힌 점. 막히지 않으면 nullopt.
     * from 근처에 걸을 수 있는 면이 없으면 from에서 막힌 것으로 본다.
     */
    optional<vector3D> Raycast(const vector3D& from, const vector3D& to);
    /** pos 아래의 바닥 높이. pos의 z에서 에이전트 높이만큼 위아래에 걸을 수 있는 면이 없으면 nullopt. */
    optional<float> FindFloorHeight(const vector3D& pos);

    /** 캡슐 중심에서 바닥까지의 높이. */
    float GetAgentHalfHeight() const { return _navMesh->GetAgent().height * 0.5f; }
    const NavMeshFile::AgentSettings& GetAgent() const { return _navMesh->GetAgent(); }

private:
    /** pos 근처의 폴리곤(dtPolyRef)과 그 위의 가장 가까운 점(Detour 좌표). 없으면 0. */
    uint32 FindNearestPoly(const vector3D& pos, OUT float* nearestPoint);
    /** 언리얼 좌표의 캡슐 중심을 Detour 좌표의 발밑으로 옮긴다. */
    void ToDetour(const vector3D& pos, OUT float* detourPos) const;
    /** Detour 좌표의 발밑을 언리얼 좌표의 캡슐 중심으로 옮긴다. */
    vector3D FromDetour(const float* detourPos) const;

    static constexpr int MAX_SEARCH_NODES = 2048;
    static constexpr int MAX_PATH_POLYS = 256;
    static constexpr int MAX_SURFACE_VISITS = 64;

    shared_ptr<const NavMesh> _navMesh;
    dtNavMeshQuery* _query = nullptr;
    unique_ptr<dtQueryFilter> _filter;
};
