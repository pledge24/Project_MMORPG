#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "Game/Navigation/NavMeshFile.h"

/**
 * 추출한 충돌 형상으로 navmesh 파일을 빌드한다.
 *
 * 맵 하나의 삼각형과 에이전트 설정을 받아 서버가 읽는 .navmesh 내용을 만들고, 에디터가 겹쳐 그리는 폴리곤을 꺼낸다.
 * 같은 입력이면 같은 바이트를 낸다. 좌표는 언리얼 단위(cm, Z가 위)로 받고 내보낸다.
 */
namespace NavMeshBuild
{
    /** 맵 하나의 충돌 형상. 삼각형은 언리얼 좌표에서 오른손 법칙으로 바깥을 향한다. */
    struct Geometry
    {
        int32_t mapId = 0;
        /** x, y, z가 차례로 이어진다. */
        std::vector<float> vertices;
        /** 꼭짓점 번호 셋이 삼각형 하나다. */
        std::vector<int32_t> triangles;
    };

    /** 추출 커맨드렛이 쓴 형상 문서(NavGeometry_<맵 번호>.json)를 읽는다. 형식이 틀리면 사유. */
    std::optional<std::string> ParseGeometry(const nlohmann::json& document, Geometry& outGeometry);

    /** navmesh 파일 내용을 만든다. 걸을 수 있는 면이 없거나 Recast가 실패하면 사유. */
    std::optional<std::string> Build(const Geometry& geometry, const NavMeshFile::AgentSettings& agent, std::vector<uint8_t>& outFile);

    /**
     * navmesh 파일의 폴리곤을 언리얼 좌표로 꺼낸다. { "MapId", "Polygons": [[[x, y, z], ...], ...] }
     * 파일이 형식에 맞지 않으면 사유.
     */
    std::optional<std::string> DumpPolygons(const std::vector<uint8_t>& file, int32_t mapId, const NavMeshFile::AgentSettings& agent, nlohmann::json& outDocument);
}
