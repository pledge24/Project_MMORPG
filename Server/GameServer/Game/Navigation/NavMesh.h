#pragma once
#include "Game/Navigation/NavMeshFile.h"

class dtNavMesh;

/**
 * 맵 하나의 navmesh.
 *
 * 서버가 기동할 때 한 번 읽고 바꾸지 않는다. 같은 맵의 룸들이 const로 함께 읽고, 질의는 룸마다 둔 NavQuery가 한다.
 */
class NavMesh
{
public:
    ~NavMesh();

    /**
     * navmesh 파일 내용을 읽는다. agent는 추출한 에이전트 설정이고 파일 머리의 해시와 대조한다.
     * 형식, 버전, 해시가 어긋나거나 Detour가 읽지 못하면 사유.
     */
    static optional<string> Load(const vector<uint8>& file, const NavMeshFile::AgentSettings& agent, OUT shared_ptr<const NavMesh>& outNavMesh);

    const dtNavMesh& GetDetour() const { return *_detour; }
    const NavMeshFile::AgentSettings& GetAgent() const { return _agent; }

private:
    NavMesh() = default;

    dtNavMesh* _detour = nullptr;
    NavMeshFile::AgentSettings _agent;
};
