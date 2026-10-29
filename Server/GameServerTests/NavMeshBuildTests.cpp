#include "Core/pch.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "Game/Navigation/NavQuery.h"
#include "Game/Room/MoveValidation.h"
#include "TestNavLevel.h"

/*--------------------------------------------------------------
    navmesh 빌드와 서버의 navmesh 읽기 테스트

    빌드 도구(Tools/NavBuilder)는 추출한 충돌 형상으로 맵마다 navmesh 파일을 쓰고, 서버는 기동할 때 그 파일을 읽는다.
    레벨이 바뀌지 않았는데 파일이 바뀌면 diff로 드러나야 하므로 같은 입력은 같은 바이트를 낸다. 형식이나 에이전트 설정이
    어긋난 파일로 서버가 뜨면 지형 판정이 조용히 틀리므로, 서버는 그런 파일로 뜨지 않는다.

    픽스처 결합도: NavMeshes와 Gamedata의 전역 표. 테스트마다 맵 표를 설치하고, navmesh는 끝에 비운다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 MAP_ID = 1;

    /** 맵 하나(MAP_ID)만 있는 맵 표를 설치한다. */
    void InstallMap(int32 mapId = MAP_ID)
    {
        GamedataTables tables;
        DesignData::MapRow mapRow;
        mapRow.Id = mapId;
        tables.maps.emplace(mapId, mapRow);
        Gamedata::Install(std::move(tables));
    }

    string FileName(int32 mapId)
    {
        return format("{}{}{}", NavMeshFile::FILE_PREFIX, mapId, NavMeshFile::FILE_EXTENSION);
    }

    NavMeshDocuments MakeDocuments()
    {
        NavMeshDocuments documents;
        documents.agent = TestNavLevel::MakeAgentDocument(TestNavLevel::MakeAgent());
        documents.files[FileName(MAP_ID)] = TestNavLevel::GetFile();
        return documents;
    }

    /** 테스트가 끝나면 navmesh와 기획 데이터를 비운다. */
    class NavMeshesTest : public ::testing::Test
    {
    protected:
        void SetUp() override { InstallMap(); }
        void TearDown() override
        {
            NavMeshes::Install({});
            Gamedata::Install(GamedataTables{});
        }
    };
}

TEST(NavMeshBuildTest, SameInputBuildsSameBytes)
{
    vector<uint8> first;
    vector<uint8> second;
    ASSERT_EQ(NavMeshBuild::Build(TestNavLevel::MakeGeometry(MAP_ID), TestNavLevel::MakeAgent(), OUT first), nullopt);
    ASSERT_EQ(NavMeshBuild::Build(TestNavLevel::MakeGeometry(MAP_ID), TestNavLevel::MakeAgent(), OUT second), nullopt);

    EXPECT_FALSE(first.empty());
    EXPECT_EQ(first, second);
}

TEST(NavMeshBuildTest, GeometryDocumentRoundTripsToSameBytes)
{
    // 추출 커맨드렛이 쓰는 문서를 읽어 빌드해도 같은 바이트다.
    const NavMeshBuild::Geometry geometry = TestNavLevel::MakeGeometry(MAP_ID);
    Json document{ { "MapId", geometry.mapId }, { "Vertices", Json::array() }, { "Triangles", Json::array() } };
    for (size_t i = 0; i < geometry.vertices.size(); i += 3)
        document["Vertices"].push_back({ geometry.vertices[i], geometry.vertices[i + 1], geometry.vertices[i + 2] });
    for (size_t i = 0; i < geometry.triangles.size(); i += 3)
        document["Triangles"].push_back({ geometry.triangles[i], geometry.triangles[i + 1], geometry.triangles[i + 2] });

    NavMeshBuild::Geometry parsed;
    ASSERT_EQ(NavMeshBuild::ParseGeometry(document, OUT parsed), nullopt);
    EXPECT_EQ(parsed.mapId, MAP_ID);

    vector<uint8> built;
    ASSERT_EQ(NavMeshBuild::Build(parsed, TestNavLevel::MakeAgent(), OUT built), nullopt);
    EXPECT_EQ(built, TestNavLevel::GetFile());
}

TEST(NavMeshBuildTest, GeometryDocumentWithMissingVertexIsRejected)
{
    const Json document{ { "MapId", MAP_ID }, { "Vertices", { { 0, 0, 0 }, { 1, 0, 0 } } }, { "Triangles", { { 0, 1, 2 } } } };

    NavMeshBuild::Geometry parsed;
    EXPECT_NE(NavMeshBuild::ParseGeometry(document, OUT parsed), nullopt);
}

TEST(NavMeshBuildTest, GeometryWithoutWalkableSurfaceIsRejected)
{
    // 벽 하나만 있으면 설 수 있는 면이 없다.
    NavMeshBuild::Geometry geometry;
    geometry.mapId = MAP_ID;
    TestNavLevel::AddBox(geometry, { 0.f, 0.f, 0.f }, { 10.f, 1000.f, 300.f });

    vector<uint8> built;
    EXPECT_NE(NavMeshBuild::Build(geometry, TestNavLevel::MakeAgent(), OUT built), nullopt);
}

TEST(NavMeshBuildTest, PolygonDumpIsOnTheFloor)
{
    // 에디터가 겹쳐 그리는 폴리곤은 언리얼 좌표다. 바닥 폴리곤의 높이는 바닥 윗면(0)이다.
    Json document;
    ASSERT_EQ(NavMeshBuild::DumpPolygons(TestNavLevel::GetFile(), MAP_ID, TestNavLevel::MakeAgent(), OUT document), nullopt);

    EXPECT_EQ(document["MapId"].get<int32>(), MAP_ID);
    ASSERT_FALSE(document["Polygons"].empty());
    bool foundFloor = false;
    for (const Json& polygon : document["Polygons"])
    {
        for (const Json& vertex : polygon)
        {
            EXPECT_GE(vertex[0].get<float>(), -1000.f);
            EXPECT_LE(vertex[0].get<float>(), 1000.f);
            if (vertex[0].get<float>() < -100.f && abs(vertex[2].get<float>()) <= 10.f)
                foundFloor = true;
        }
    }
    EXPECT_TRUE(foundFloor) << "벽 왼쪽 바닥(z 0)의 폴리곤이 없다";
}

TEST_F(NavMeshesTest, NavMeshForEveryMapIsInstalled)
{
    ASSERT_EQ(NavMeshes::Load(MakeDocuments()), nullopt);

    EXPECT_NE(NavMeshes::Find(MAP_ID), nullptr);
    EXPECT_EQ(NavMeshes::Find(MAP_ID + 1), nullptr);
}

TEST_F(NavMeshesTest, DifferentFormatVersionIsRejected)
{
    NavMeshDocuments documents = MakeDocuments();
    documents.files[FileName(MAP_ID)][4] = static_cast<uint8>(NavMeshFile::FORMAT_VERSION + 1);

    const optional<string> error = NavMeshes::Load(documents);

    ASSERT_NE(error, nullopt);
    EXPECT_NE(error->find(FileName(MAP_ID)), string::npos) << error.value();
    EXPECT_NE(error->find("형식 버전"), string::npos) << error.value();
    EXPECT_EQ(NavMeshes::Find(MAP_ID), nullptr) << "실패하면 아무것도 설치하지 않는다";
}

TEST_F(NavMeshesTest, DifferentAgentSettingsAreRejected)
{
    // 플레이어 캡슐을 키우고 추출만 다시 돌린 상태다. 옛 navmesh는 좁은 통로를 지나갈 수 있다고 답한다.
    NavMeshFile::AgentSettings changed = TestNavLevel::MakeAgent();
    changed.radius = 50.f;
    NavMeshDocuments documents = MakeDocuments();
    documents.agent = TestNavLevel::MakeAgentDocument(changed);

    const optional<string> error = NavMeshes::Load(documents);

    ASSERT_NE(error, nullopt);
    EXPECT_NE(error->find("해시"), string::npos) << error.value();
}

TEST_F(NavMeshesTest, MissingNavMeshOfMapIsRejected)
{
    GamedataTables tables;
    for (int32 mapId : { MAP_ID, MAP_ID + 1 })
    {
        DesignData::MapRow mapRow;
        mapRow.Id = mapId;
        tables.maps.emplace(mapId, mapRow);
    }
    Gamedata::Install(std::move(tables));

    const optional<string> error = NavMeshes::Load(MakeDocuments());

    ASSERT_NE(error, nullopt);
    EXPECT_NE(error->find(FileName(MAP_ID + 1)), string::npos) << error.value();
}

TEST_F(NavMeshesTest, NavMeshOfUnknownMapIsRejected)
{
    NavMeshDocuments documents = MakeDocuments();
    documents.files[FileName(MAP_ID + 1)] = TestNavLevel::GetFile();

    EXPECT_NE(NavMeshes::Load(documents), nullopt);
}

TEST_F(NavMeshesTest, AgentDocumentWithMissingFieldIsRejected)
{
    NavMeshDocuments documents = MakeDocuments();
    documents.agent.erase("MaxStep");

    const optional<string> error = NavMeshes::Load(documents);

    ASSERT_NE(error, nullopt);
    EXPECT_NE(error->find("MaxStep"), string::npos) << error.value();
}

TEST_F(NavMeshesTest, RepositoryNavMeshesPass)
{
    // 테스트 소스 위치에서 저장소의 추출 산출물 폴더를 찾는다. 작업 디렉터리와 무관하게 돈다.
    const filesystem::path dataDir = filesystem::path(__FILE__).parent_path() / ".." / "GameServer" / "Game" / "Data";

    GamedataTables tables;
    ifstream mapFile(dataDir / "Generated" / DesignData::MAP_FILE);
    ASSERT_TRUE(mapFile.is_open());
    for (const Json& row : Json::parse(mapFile))
    {
        DesignData::MapRow mapRow;
        mapRow.Id = row["Id"].get<int32>();
        tables.maps.emplace(mapRow.Id, mapRow);
    }
    Gamedata::Install(std::move(tables));

    NavMeshDocuments documents;
    ifstream agentFile(dataDir / "Extracted" / NavMeshFile::AGENT_FILE);
    ASSERT_TRUE(agentFile.is_open()) << "추출한 에이전트 설정이 없다";
    documents.agent = Json::parse(agentFile);
    for (const filesystem::directory_entry& entry : filesystem::directory_iterator(dataDir / "Extracted"))
    {
        const string fileName = entry.path().filename().string();
        if (fileName.starts_with(NavMeshFile::FILE_PREFIX) == false)
            continue;

        ifstream file(entry.path(), ios::binary);
        documents.files[fileName].assign(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
    }

    const optional<string> error = NavMeshes::Load(documents);
    EXPECT_FALSE(error.has_value()) << error.value_or("");

    // 플레이어 캡슐(크리처 판정 데이터의 전사)과 같은 에이전트다.
    NavMeshFile::AgentSettings agent;
    ASSERT_EQ(NavMeshFile::ParseAgent(documents.agent, OUT agent), nullopt);
    EXPECT_FLOAT_EQ(agent.radius, 42.f);
    EXPECT_FLOAT_EQ(agent.height, 192.f);

    // 마을(지역 10)은 x, y가 -1500 ~ 1500인 섬이고 바닥 윗면이 0이다. 섬 밖으로 나가는 이동은 거절한다.
    constexpr int32 MAP_ID_IN_GAME = 1111;
    const shared_ptr<const NavMesh> navMesh = NavMeshes::Find(MAP_ID_IN_GAME);
    ASSERT_NE(navMesh, nullptr);
    NavQuery query(navMesh);
    constexpr float STANDING_Z = 96.f;

    const optional<float> townFloor = query.FindFloorHeight({ 0.f, -500.f, STANDING_Z });
    ASSERT_TRUE(townFloor.has_value()) << "마을 가운데에 바닥이 없다";
    EXPECT_NEAR(townFloor.value(), 0.f, 15.f);

    const PlaneBounds mapBounds{ -13000.f, 2000.f, -13000.f, 13000.f };
    EXPECT_EQ(MoveValidation::Validate({ 0.f, -500.f, STANDING_Z }, { 200.f, -500.f, STANDING_Z }, 200, mapBounds, &query), nullopt);
    EXPECT_NE(MoveValidation::Validate({ 1400.f, -500.f, STANDING_Z }, { 1600.f, -500.f, STANDING_Z }, 200, mapBounds, &query), nullopt)
        << "마을 섬의 경계 벽 너머로 나갔다";
}
