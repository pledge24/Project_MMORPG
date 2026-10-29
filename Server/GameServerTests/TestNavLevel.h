#pragma once
#include "NavMeshBuild.h"
#include "Game/Navigation/NavMesh.h"
#include "Game/Navigation/NavMeshes.h"

/*--------------------------------------------------------------
    navmesh 테스트 레벨

    가운데에 벽이 하나 선 바닥과 경사로 하나. 추출 커맨드렛과 같은 규칙(볼록 형상의 삼각형은 형상 중심의
    바깥을 향한다)으로 형상을 만들어 빌드 도구로 navmesh를 빌드한다.

    바닥   x[-1000, 1000] y[-1000, 1000], 윗면 z = 0
    벽     x[-50, 50] y[-1000, 400], z[0, 300]. y 400 ~ 1000은 비어 있어 벽을 돌아갈 수 있다
    경사로 x[500, 900] y[-800, -400]. x 500에서 z 0, x 900에서 z 100까지 오른다(약 14도)
---------------------------------------------------------------*/

namespace TestNavLevel
{
    /** 플레이어 캡슐(반경 42, 반높이 96)과 이동 컴포넌트의 기본값. */
    inline NavMeshFile::AgentSettings MakeAgent()
    {
        return NavMeshFile::AgentSettings{ 42.f, 192.f, 44.77f, 45.f };
    }

    inline Json MakeAgentDocument(const NavMeshFile::AgentSettings& agent)
    {
        return Json{ { "Radius", agent.radius }, { "Height", agent.height }, { "MaxSlope", agent.maxSlope }, { "MaxStep", agent.maxStep } };
    }

    /** 볼록 형상 하나를 더한다. 삼각형마다 형상 중심의 바깥을 향하게 꼭짓점 순서를 맞춘다. */
    inline void AddConvex(NavMeshBuild::Geometry& geometry, const vector<vector3D>& corners, const vector<array<int32, 3>>& triangles)
    {
        vector3D center;
        for (const vector3D& corner : corners)
        {
            center.x += corner.x / corners.size();
            center.y += corner.y / corners.size();
            center.z += corner.z / corners.size();
        }

        const int32 base = static_cast<int32>(geometry.vertices.size() / 3);
        for (const vector3D& corner : corners)
        {
            geometry.vertices.push_back(corner.x);
            geometry.vertices.push_back(corner.y);
            geometry.vertices.push_back(corner.z);
        }

        for (array<int32, 3> triangle : triangles)
        {
            const vector3D& a = corners[triangle[0]];
            const vector3D& b = corners[triangle[1]];
            const vector3D& c = corners[triangle[2]];
            const vector3D ab(b.x - a.x, b.y - a.y, b.z - a.z);
            const vector3D ac(c.x - a.x, c.y - a.y, c.z - a.z);
            const vector3D normal(ab.y * ac.z - ab.z * ac.y, ab.z * ac.x - ab.x * ac.z, ab.x * ac.y - ab.y * ac.x);
            const float outward = normal.x * (a.x - center.x) + normal.y * (a.y - center.y) + normal.z * (a.z - center.z);
            if (outward < 0.f)
                swap(triangle[1], triangle[2]);

            for (int32 index : triangle)
                geometry.triangles.push_back(base + index);
        }
    }

    inline void AddBox(NavMeshBuild::Geometry& geometry, const vector3D& min, const vector3D& max)
    {
        const vector<vector3D> corners = {
            { min.x, min.y, min.z }, { max.x, min.y, min.z }, { max.x, max.y, min.z }, { min.x, max.y, min.z },
            { min.x, min.y, max.z }, { max.x, min.y, max.z }, { max.x, max.y, max.z }, { min.x, max.y, max.z },
        };
        AddConvex(geometry, corners, {
            { 0, 1, 2 }, { 0, 2, 3 }, { 4, 5, 6 }, { 4, 6, 7 },
            { 0, 1, 5 }, { 0, 5, 4 }, { 1, 2, 6 }, { 1, 6, 5 },
            { 2, 3, 7 }, { 2, 7, 6 }, { 3, 0, 4 }, { 3, 4, 7 },
        });
    }

    /** x 방향으로 오르는 쐐기. 아래 면은 z = baseZ, 위 면은 x = min.x에서 baseZ, x = max.x에서 topZ다. */
    inline void AddRamp(NavMeshBuild::Geometry& geometry, const vector2D& min, const vector2D& max, float baseZ, float topZ)
    {
        const vector<vector3D> corners = {
            { min.x, min.y, baseZ }, { max.x, min.y, baseZ }, { max.x, max.y, baseZ }, { min.x, max.y, baseZ },
            { max.x, min.y, topZ }, { max.x, max.y, topZ },
        };
        AddConvex(geometry, corners, {
            { 0, 1, 2 }, { 0, 2, 3 },
            { 0, 4, 5 }, { 0, 5, 3 },
            { 1, 2, 5 }, { 1, 5, 4 },
            { 0, 1, 4 }, { 3, 2, 5 },
        });
    }

    inline NavMeshBuild::Geometry MakeGeometry(int32 mapId)
    {
        NavMeshBuild::Geometry geometry;
        geometry.mapId = mapId;
        AddBox(geometry, { -1000.f, -1000.f, -50.f }, { 1000.f, 1000.f, 0.f });
        AddBox(geometry, { -50.f, -1000.f, 0.f }, { 50.f, 400.f, 300.f });
        AddRamp(geometry, { 500.f, -800.f }, { 900.f, -400.f }, 0.f, 100.f);
        return geometry;
    }

    /** 테스트 레벨의 navmesh 파일. 빌드는 한 번만 한다. */
    inline const vector<uint8>& GetFile()
    {
        static const vector<uint8> file = []
        {
            vector<uint8> built;
            const optional<string> error = NavMeshBuild::Build(MakeGeometry(1), MakeAgent(), OUT built);
            if (error.has_value())
                ADD_FAILURE() << "테스트 레벨의 navmesh를 빌드하지 못했다: " << error.value();
            return built;
        }();
        return file;
    }

    inline shared_ptr<const NavMesh> Load()
    {
        shared_ptr<const NavMesh> navMesh;
        const optional<string> error = NavMesh::Load(GetFile(), MakeAgent(), OUT navMesh);
        if (error.has_value())
            ADD_FAILURE() << "테스트 레벨의 navmesh를 읽지 못했다: " << error.value();
        return navMesh;
    }

    /** 바닥 위의 캡슐 중심 높이. */
    constexpr float STANDING_Z = 96.f;
}

/** 테스트 동안 navmesh를 설치하고 끝나면 비운다. 다른 테스트의 룸이 이 navmesh로 지형을 검사하지 않게 한다. */
class ScopedNavMeshes
{
public:
    explicit ScopedNavMeshes(map<int32, shared_ptr<const NavMesh>> navMeshes) { NavMeshes::Install(std::move(navMeshes)); }
    ~ScopedNavMeshes() { NavMeshes::Install({}); }
};
