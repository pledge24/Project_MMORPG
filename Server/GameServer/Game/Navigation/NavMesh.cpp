#include "Core/pch.h"
#include "Game/Navigation/NavMesh.h"
#include "DetourAlloc.h"
#include "DetourNavMesh.h"

NavMesh::~NavMesh()
{
    dtFreeNavMesh(_detour);
}

optional<string> NavMesh::Load(const vector<uint8>& file, const NavMeshFile::AgentSettings& agent, OUT shared_ptr<const NavMesh>& outNavMesh)
{
    NavMeshFile::TileData tile;
    if (optional<string> error = NavMeshFile::Read(file, NavMeshFile::HashAgent(agent), tile))
        return error;

    shared_ptr<NavMesh> navMesh(new NavMesh());
    navMesh->_agent = agent;
    navMesh->_detour = dtAllocNavMesh();
    if (navMesh->_detour == nullptr)
        return string("Detour navmesh를 할당하지 못했다");

    // Detour가 타일 데이터를 갖고 해제하므로 Detour의 할당자로 복사해 넘긴다.
    unsigned char* data = static_cast<unsigned char*>(dtAlloc(tile.size, DT_ALLOC_PERM));
    if (data == nullptr)
        return string("타일 데이터를 할당하지 못했다");
    memcpy(data, tile.data, tile.size);

    if (dtStatusFailed(navMesh->_detour->init(data, static_cast<int>(tile.size), DT_TILE_FREE_DATA)))
    {
        dtFree(data);
        return string("Detour가 타일 데이터를 읽지 못했다");
    }

    outNavMesh = std::move(navMesh);
    return nullopt;
}
