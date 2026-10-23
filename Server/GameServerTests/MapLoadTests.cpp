#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "RecordingSession.h"
#include "RoomManagerTestAccess.h"

/*--------------------------------------------------------------
    맵 로드 절차 테스트

    맵이 바뀌는 입장(첫 입장, 다른 맵으로 가는 포털, 다른 맵으로의 리스폰)은 서버가 열 맵을 알리고, 클라이언트가
    그 맵을 다 열었다고 알린 뒤에야 룸에 들어간다. 전에는 첫 입장만 이 순서를 지켰고, 맵 간 이동은 맵 입장 요청과
    룸 입장 요청 두 단계로 나뉘어 클라이언트가 보내지 않는 경로로 남아 있었다.

    픽스처 결합도: 맵 표(MAP_A의 마을과 사냥터, MAP_B의 룸 하나)와 1레벨 전사 레벨 표를 Gamedata::Install로 주입하고,
    맵 표의 행마다 시작하지 않은 룸을 둔 룸 관리자를 GRoomManager로 쓴다. 룸 큐의 잡은 테스트 스레드에서 곧바로 돈다.
    플레이어는 패킷을 쌓는 RecordingSession에 묶는다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 MAP_A = 1111;
    constexpr int32 MAP_B = 2222;
    constexpr int32 TOWN_ROOM = 10;
    constexpr int32 FIELD_ROOM = 20;
    constexpr int32 OTHER_MAP_ROOM = 50;
    constexpr int32 PORTAL_TO_FIELD = 11;
    constexpr int32 PORTAL_TO_OTHER_MAP = 12;
    constexpr float PORTAL_RADIUS = 500.f;
    constexpr float FIELD_ARRIVAL_X = -9000.f;
    constexpr float OTHER_MAP_ARRIVAL_X = 7000.f;
    constexpr float TOWN_RESPAWN_X = 300.f;
    constexpr int32 PLAYER_MAX_HP = 100;

    Protocol::PosInfo MakePos(float x, float y)
    {
        Protocol::PosInfo pos;
        pos.mutable_pos()->set_x(x);
        pos.mutable_pos()->set_y(y);
        pos.set_state(Protocol::MOVE_STATE_IDLE);
        return pos;
    }

    MapTemplate MakeRoomTemplate(int32 roomId, int32 mapId, float centerX)
    {
        MapTemplate room;
        room.templateId = roomId;
        room.mapId = mapId;
        room.center = TemplatePos{ centerX, 0.f, 0.f };
        room.depthHalfExtent = 3000.f;
        room.widthHalfExtent = 3000.f;
        room.portalRadius = PORTAL_RADIUS;
        return room;
    }

    PortalTemplate MakePortal(int32 portalId, int32 dstRoomId, float srcX, float dstX)
    {
        PortalTemplate portal;
        portal.portalId = portalId;
        portal.dstRoomId = dstRoomId;
        portal.srcPos = TemplatePos{ srcX, 0.f, 0.f };
        portal.dstPos = TemplatePos{ dstX, 0.f, 0.f };
        return portal;
    }

    Protocol::C_USE_PORTAL MakeUsePortal(int32 portalId)
    {
        Protocol::C_USE_PORTAL pkt;
        pkt.set_portal_id(portalId);
        return pkt;
    }

    Protocol::C_RESPAWN MakeTownRespawn()
    {
        Protocol::C_RESPAWN pkt;
        pkt.set_respawn_type(Protocol::RESPAWN_TYPE_TOWN);
        return pkt;
    }
}

class MapLoadTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 마을에는 같은 맵의 사냥터로 가는 포털과 다른 맵의 룸으로 가는 포털이 있다.
        MapTemplate town = MakeRoomTemplate(TOWN_ROOM, MAP_A, 0.f);
        town.respawnPoint = TemplatePos{ TOWN_RESPAWN_X, 0.f, 0.f };
        town.portals.push_back(MakePortal(PORTAL_TO_FIELD, FIELD_ROOM, -1000.f, FIELD_ARRIVAL_X));
        town.portals.push_back(MakePortal(PORTAL_TO_OTHER_MAP, OTHER_MAP_ROOM, 1000.f, OTHER_MAP_ARRIVAL_X));

        LevelTemplate level1;
        level1.level = 1;
        level1.maxHp = PLAYER_MAX_HP;

        GamedataTables tables;
        tables.maps[TOWN_ROOM] = town;
        tables.maps[FIELD_ROOM] = MakeRoomTemplate(FIELD_ROOM, MAP_A, -10000.f);
        tables.maps[OTHER_MAP_ROOM] = MakeRoomTemplate(OTHER_MAP_ROOM, MAP_B, 7000.f);
        tables.townRoomId = TOWN_ROOM;
        tables.classLevelTables[Protocol::CHARACTER_CLASS_WARRIOR] = ClassLevelTable({ level1 });
        Gamedata::Install(std::move(tables));

        rooms = make_unique<ScopedTestRoomManager>();
        townRoom = rooms->FindRoom(TOWN_ROOM);
        fieldRoom = rooms->FindRoom(FIELD_ROOM);
        otherMapRoom = rooms->FindRoom(OTHER_MAP_ROOM);
    }

    void TearDown() override
    {
        townRoom.reset();
        fieldRoom.reset();
        otherMapRoom.reset();
        rooms.reset();
        sessions.clear();
        Gamedata::Install(GamedataTables());
    }

    /** 세션에 묶인 1레벨 전사. 아직 어느 룸에도 없다. */
    PlayerRef CreatePlayer(shared_ptr<RecordingSession>& outSession)
    {
        outSession = make_shared<RecordingSession>();
        sessions.push_back(outSession);

        PlayerProgress progress;
        progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
        progress.level = 1;
        progress.hp = PLAYER_MAX_HP;

        PlayerSpawnParams params;
        params.session = outSession;
        params.progress = &progress;
        return EntityFactory::Create<Player>(params);
    }

    /** room의 pos에 들어간 플레이어. 입장까지 받은 패킷은 비운다. */
    PlayerRef CreatePlayerIn(const RoomRef& room, const Protocol::PosInfo& pos, shared_ptr<RecordingSession>& outSession)
    {
        PlayerRef player = CreatePlayer(outSession);
        player->BeginMapLoad(room, pos);
        room->C_HandleMapLoadComplete(player);
        outSession->ClearSent();
        return player;
    }

    static void Kill(const PlayerRef& player)
    {
        AttackInfo lethalAttack;
        lethalAttack.damage = PLAYER_MAX_HP * 10;
        player->OnHit(nullptr, lethalAttack);
    }

    unique_ptr<ScopedTestRoomManager> rooms;
    vector<shared_ptr<RecordingSession>> sessions;
    RoomRef townRoom;
    RoomRef fieldRoom;
    RoomRef otherMapRoom;
};

// 게임 입장 응답을 받은 클라이언트는 맵을 다 연 뒤에 로드 완료를 보낸다. 그 전에 룸에 넣으면 클라이언트가 스폰을
// 받을 월드가 없다.
TEST_F(MapLoadTest, FirstEntryEntersRoomAfterMapLoadComplete)
{
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayer(session);
    player->BeginMapLoad(townRoom, MakePos(100.f, 0.f));

    EXPECT_FALSE(townRoom->Contains(player->GetEntityId())) << "맵 로드 완료 전에는 룸에 없다";

    townRoom->C_HandleMapLoadComplete(player);

    EXPECT_TRUE(townRoom->Contains(player->GetEntityId()));
    const vector<Protocol::S_ENTER_ROOM> responses = session->SentPackets<Protocol::S_ENTER_ROOM>(PKT_S_ENTER_ROOM);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_SUCCESS);
    EXPECT_EQ(responses[0].my_entity().entity_id(), player->GetEntityId());
    EXPECT_FLOAT_EQ(responses[0].my_entity().pos_info().pos().x(), 100.f);
}

// 내 엔티티는 룸 입장 응답에만 실린다. 스폰에도 실리면 클라이언트가 내 플레이어를 두 번 처리한다.
TEST_F(MapLoadTest, MyEntityComesOnlyWithEnterRoom)
{
    shared_ptr<RecordingSession> residentSession;
    PlayerRef resident = CreatePlayerIn(townRoom, MakePos(0.f, 0.f), residentSession);

    shared_ptr<RecordingSession> session;
    PlayerRef newcomer = CreatePlayer(session);
    newcomer->BeginMapLoad(townRoom, MakePos(100.f, 0.f));
    townRoom->C_HandleMapLoadComplete(newcomer);

    vector<int64> spawnedForNewcomer;
    for (const Protocol::S_SPAWN& spawnPkt : session->SentPackets<Protocol::S_SPAWN>(PKT_S_SPAWN))
        for (const Protocol::EntityInfo& entity : spawnPkt.entities())
            spawnedForNewcomer.push_back(entity.entity_id());

    EXPECT_EQ(spawnedForNewcomer, vector<int64>({ resident->GetEntityId() })) << "다른 엔티티만 스폰으로 온다";
    EXPECT_EQ(residentSession->SentPackets<Protocol::S_SPAWN>(PKT_S_SPAWN).size(), 1u) << "룸에 있던 플레이어는 새로 온 플레이어의 스폰을 받는다";
}

// 맵 로드 완료 전에 온 룸 요청은 룸에 없는 플레이어의 요청이므로 버린다.
TEST_F(MapLoadTest, RoomRequestsBeforeMapLoadCompleteAreDropped)
{
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayer(session);
    player->BeginMapLoad(townRoom, MakePos(1000.f, 0.f));

    Protocol::C_MOVE movePkt;
    *movePkt.mutable_pos_info() = MakePos(1050.f, 0.f);
    townRoom->C_HandleMove(movePkt, player);
    townRoom->C_HandleUsePortal(MakeUsePortal(PORTAL_TO_OTHER_MAP), player);
    Kill(player);
    townRoom->C_HandleRespawn(MakeTownRespawn(), player);

    EXPECT_FLOAT_EQ(player->GetPosInfo().pos().x(), 1000.f) << "이동을 받으면 룸에 없는 위치가 저장된다";
    EXPECT_TRUE(session->SentPackets<Protocol::S_USE_PORTAL>(PKT_S_USE_PORTAL).empty());
    EXPECT_TRUE(session->SentPackets<Protocol::S_RESPAWN>(PKT_S_RESPAWN).empty());
    EXPECT_FALSE(otherMapRoom->Contains(player->GetEntityId()));
}

// 룸에 있는 플레이어가 로드 완료를 다시 보내도 한 번 더 들어가지 않는다.
TEST_F(MapLoadTest, MapLoadCompleteWithoutWaitingIsDropped)
{
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayerIn(townRoom, MakePos(0.f, 0.f), session);

    townRoom->C_HandleMapLoadComplete(player);

    EXPECT_TRUE(session->SentPackets<Protocol::S_ENTER_ROOM>(PKT_S_ENTER_ROOM).empty());
    EXPECT_TRUE(townRoom->Contains(player->GetEntityId()));
}

TEST_F(MapLoadTest, PortalToOtherMapEntersAfterMapLoadComplete)
{
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayerIn(townRoom, MakePos(1000.f, 0.f), session);

    townRoom->C_HandleUsePortal(MakeUsePortal(PORTAL_TO_OTHER_MAP), player);

    const vector<Protocol::S_USE_PORTAL> responses = session->SentPackets<Protocol::S_USE_PORTAL>(PKT_S_USE_PORTAL);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_SUCCESS);
    ASSERT_TRUE(responses[0].has_map_id());
    EXPECT_EQ(responses[0].map_id(), MAP_B);
    EXPECT_FALSE(townRoom->Contains(player->GetEntityId()));
    EXPECT_FALSE(otherMapRoom->Contains(player->GetEntityId())) << "클라이언트가 그 맵을 다 열기 전에는 들어가지 않는다";
    EXPECT_TRUE(session->SentPackets<Protocol::S_ENTER_ROOM>(PKT_S_ENTER_ROOM).empty());

    otherMapRoom->C_HandleMapLoadComplete(player);

    EXPECT_TRUE(otherMapRoom->Contains(player->GetEntityId()));
    EXPECT_EQ(player->GetMapId(), MAP_B);
    const vector<Protocol::S_ENTER_ROOM> enterRooms = session->SentPackets<Protocol::S_ENTER_ROOM>(PKT_S_ENTER_ROOM);
    ASSERT_EQ(enterRooms.size(), 1u);
    EXPECT_FLOAT_EQ(enterRooms[0].my_entity().pos_info().pos().x(), OTHER_MAP_ARRIVAL_X) << "포털의 도착 위치로 들어간다";
}

// 같은 맵 안의 포털 이동은 클라이언트가 레벨을 다시 열지 않으므로 로드 완료를 기다리지 않는다.
TEST_F(MapLoadTest, PortalWithinSameMapEntersWithoutMapLoad)
{
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayerIn(townRoom, MakePos(-1000.f, 0.f), session);

    townRoom->C_HandleUsePortal(MakeUsePortal(PORTAL_TO_FIELD), player);

    const vector<Protocol::S_USE_PORTAL> responses = session->SentPackets<Protocol::S_USE_PORTAL>(PKT_S_USE_PORTAL);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_SUCCESS);
    EXPECT_FALSE(responses[0].has_map_id());
    EXPECT_TRUE(fieldRoom->Contains(player->GetEntityId()));
    EXPECT_EQ(session->SentPackets<Protocol::S_ENTER_ROOM>(PKT_S_ENTER_ROOM).size(), 1u);
}

TEST_F(MapLoadTest, PortalOutsideRadiusIsRejected)
{
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayerIn(townRoom, MakePos(0.f, 0.f), session);

    townRoom->C_HandleUsePortal(MakeUsePortal(PORTAL_TO_OTHER_MAP), player);
    townRoom->C_HandleUsePortal(MakeUsePortal(99), player);

    const vector<Protocol::S_USE_PORTAL> responses = session->SentPackets<Protocol::S_USE_PORTAL>(PKT_S_USE_PORTAL);
    ASSERT_EQ(responses.size(), 2u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_TOO_FAR_FROM_PORTAL);
    EXPECT_EQ(responses[1].result(), Protocol::ENTRY_RESULT_INVALID_PORTAL);
    EXPECT_TRUE(townRoom->Contains(player->GetEntityId()));
}

TEST_F(MapLoadTest, RespawnToOtherMapEntersAfterMapLoadComplete)
{
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayerIn(otherMapRoom, MakePos(7000.f, 0.f), session);
    Kill(player);

    otherMapRoom->C_HandleRespawn(MakeTownRespawn(), player);

    const vector<Protocol::S_RESPAWN> responses = session->SentPackets<Protocol::S_RESPAWN>(PKT_S_RESPAWN);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_SUCCESS);
    ASSERT_TRUE(responses[0].has_map_id());
    EXPECT_EQ(responses[0].map_id(), MAP_A);
    EXPECT_FALSE(responses[0].changed_stats().empty()) << "리스폰으로 회복한 스탯을 싣는다";
    EXPECT_FALSE(otherMapRoom->Contains(player->GetEntityId()));
    EXPECT_FALSE(townRoom->Contains(player->GetEntityId())) << "마을 맵을 다 열기 전에는 들어가지 않는다";

    townRoom->C_HandleMapLoadComplete(player);

    EXPECT_TRUE(townRoom->Contains(player->GetEntityId()));
    EXPECT_FALSE(player->IsDead());
    const vector<Protocol::S_ENTER_ROOM> enterRooms = session->SentPackets<Protocol::S_ENTER_ROOM>(PKT_S_ENTER_ROOM);
    ASSERT_EQ(enterRooms.size(), 1u);
    EXPECT_FLOAT_EQ(enterRooms[0].my_entity().pos_info().pos().x(), TOWN_RESPAWN_X);
}

// 사망한 룸으로 리스폰해도 퇴장과 입장을 거친다. 다른 플레이어는 사망한 모습을 지우고 리스폰한 모습을 받는다.
TEST_F(MapLoadTest, RespawnInSameRoomLeavesAndEntersAgain)
{
    shared_ptr<RecordingSession> watcherSession;
    CreatePlayerIn(townRoom, MakePos(0.f, 500.f), watcherSession);
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayerIn(townRoom, MakePos(0.f, 0.f), session);
    watcherSession->ClearSent();
    Kill(player);

    townRoom->C_HandleRespawn(MakeTownRespawn(), player);

    const vector<Protocol::S_RESPAWN> responses = session->SentPackets<Protocol::S_RESPAWN>(PKT_S_RESPAWN);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_FALSE(responses[0].has_map_id()) << "맵이 같으면 레벨을 다시 열지 않는다";
    EXPECT_TRUE(townRoom->Contains(player->GetEntityId()));
    EXPECT_EQ(session->SentPackets<Protocol::S_ENTER_ROOM>(PKT_S_ENTER_ROOM).size(), 1u);
    EXPECT_EQ(watcherSession->SentPackets<Protocol::S_DESPAWN>(PKT_S_DESPAWN).size(), 1u);
    EXPECT_EQ(watcherSession->SentPackets<Protocol::S_SPAWN>(PKT_S_SPAWN).size(), 1u);
}
