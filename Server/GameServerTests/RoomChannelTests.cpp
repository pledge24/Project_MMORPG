#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "RecordingSession.h"
#include "RoomManagerTestAccess.h"
#include "TestLevelTables.h"
#include "TestAreaTables.h"

/*--------------------------------------------------------------
    채널 테스트

    룸은 맵 하나 전체의 사본이고, 같은 맵의 룸들은 플레이어에게 채널로 보인다. 입장과 맵 간 이동은 그 맵에서 인원이
    가장 적은 채널로 가고, 채널 변경은 맵을 다시 열지 않는 룸 이동이다. 채널은 DB에 저장하지 않는다.

    픽스처 결합도: 지역 표(MAP_A의 마을, MAP_B의 사냥터)와 1레벨 전사 레벨 표를 Gamedata::Install로 주입하고,
    맵마다 시작하지 않은 룸 둘(1채널과 2채널)을 둔 룸 관리자를 GRoomManager로 쓴다. 룸 큐의 잡은 테스트 스레드에서
    곧바로 돈다. 맵의 셀은 넓어서 같은 룸의 플레이어는 서로를 본다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 MAP_A = 1111;
    constexpr int32 MAP_B = 2222;
    constexpr int32 TOWN_AREA = 10;
    constexpr int32 FIELD_AREA = 50;
    constexpr int32 PORTAL_TO_FIELD = 12;
    constexpr int32 CHANNEL_COUNT = 2;
    constexpr int32 PLAYER_MAX_HP = 100;

    Protocol::PosInfo MakePos(float x, float y)
    {
        Protocol::PosInfo pos;
        pos.mutable_pos()->set_x(x);
        pos.mutable_pos()->set_y(y);
        pos.set_state(Protocol::MOVE_STATE_IDLE);
        return pos;
    }

    Protocol::C_CHANGE_CHANNEL MakeChangeChannel(int32 channel)
    {
        Protocol::C_CHANGE_CHANNEL pkt;
        pkt.set_channel(channel);
        return pkt;
    }
}

class RoomChannelTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        TestAreaTables::TestArea town = TestAreaTables::MakeArea(TOWN_AREA, MAP_A, 0.f, 0.f, 3000.f);
        TestAreaTables::SetRespawnPoint(town, 0.f, 0.f, 0.f);
        TestAreaTables::AddPortal(town, PORTAL_TO_FIELD, FIELD_AREA, 1000.f, 0.f, 500.f).dest.x = 7000.f;

        GamedataTables tables;
        TestAreaTables::AddTown(tables, town);
        TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(FIELD_AREA, MAP_B, 7000.f, 0.f, 3000.f));
        TestLevelTables::AddWarriorLevel(tables, 1).MaxHp = PLAYER_MAX_HP;
        Gamedata::Install(std::move(tables));

        rooms = make_unique<ScopedTestRoomManager>(CHANNEL_COUNT);
        townChannel1 = rooms->FindRoom(MAP_A, 1);
        townChannel2 = rooms->FindRoom(MAP_A, 2);
        fieldChannel1 = rooms->FindRoom(MAP_B, 1);
        fieldChannel2 = rooms->FindRoom(MAP_B, 2);
        ASSERT_NE(townChannel1, nullptr);
        ASSERT_NE(townChannel2, nullptr);
        ASSERT_NE(fieldChannel1, nullptr);
        ASSERT_NE(fieldChannel2, nullptr);
    }

    void TearDown() override
    {
        townChannel1.reset();
        townChannel2.reset();
        fieldChannel1.reset();
        fieldChannel2.reset();
        rooms.reset();
        sessions.clear();
        Gamedata::Install(GamedataTables());
    }

    /** room의 pos에 들어간 1레벨 전사. 입장까지 받은 패킷은 비운다. */
    PlayerRef CreatePlayerIn(const RoomRef& room, const Protocol::PosInfo& pos, shared_ptr<RecordingSession>& outSession)
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
        PlayerRef player = EntityFactory::Create<Player>(params);

        player->BeginMapLoad(room, pos);
        room->C_HandleMapLoadComplete(player);
        outSession->ClearSent();
        return player;
    }

    static vector<Protocol::S_CHANGE_CHANNEL> ChangeChannelResponses(const shared_ptr<RecordingSession>& session)
    {
        return session->SentPackets<Protocol::S_CHANGE_CHANNEL>(PKT_S_CHANGE_CHANNEL);
    }

    unique_ptr<ScopedTestRoomManager> rooms;
    vector<shared_ptr<RecordingSession>> sessions;
    RoomRef townChannel1;
    RoomRef townChannel2;
    RoomRef fieldChannel1;
    RoomRef fieldChannel2;
};

TEST_F(RoomChannelTest, EveryMapGetsChannelCountRooms)
{
    const vector<RoomRef> made = RoomManager::MakeRooms(3);

    ASSERT_EQ(made.size(), 6u) << "맵 둘 × 채널 셋";
    set<int32> roomIds;
    for (size_t i = 0; i < made.size(); i++)
    {
        EXPECT_EQ(made[i]->GetMapId(), i < 3 ? MAP_A : MAP_B) << "맵 번호 순서다";
        EXPECT_EQ(made[i]->GetChannel(), static_cast<int32>(i % 3) + 1) << "맵 안에서는 1채널부터다";
        roomIds.insert(made[i]->GetRoomId());
    }
    EXPECT_EQ(roomIds.size(), made.size()) << "룸 번호는 서버 안에서 겹치지 않는다";
}

TEST_F(RoomChannelTest, MapWithoutPlacementMakesNoRoom)
{
    GamedataTables tables;
    tables.maps[MAP_A].Id = MAP_A;
    Gamedata::Install(std::move(tables));

    EXPECT_TRUE(RoomManager::MakeRooms(CHANNEL_COUNT).empty()) << "경계를 모르는 맵의 룸은 셀을 만들 수 없다";
}

TEST_F(RoomChannelTest, SelectRoomPicksLeastPopulatedChannel)
{
    shared_ptr<RecordingSession> session;
    CreatePlayerIn(townChannel1, MakePos(0.f, 0.f), session);

    EXPECT_EQ(GRoomManager->SelectRoom(MAP_A), townChannel2) << "1채널에 한 명이 있으면 빈 2채널로 간다";

    CreatePlayerIn(townChannel2, MakePos(0.f, 0.f), session);

    EXPECT_EQ(GRoomManager->SelectRoom(MAP_A), townChannel1) << "인원이 같으면 1채널이다";
}

TEST_F(RoomChannelTest, PortalToOtherMapGoesToLeastPopulatedChannel)
{
    shared_ptr<RecordingSession> occupantSession;
    CreatePlayerIn(fieldChannel1, MakePos(7000.f, 0.f), occupantSession);
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayerIn(townChannel1, MakePos(1000.f, 0.f), session);

    Protocol::C_USE_PORTAL portalPkt;
    portalPkt.set_portal_id(PORTAL_TO_FIELD);
    townChannel1->C_HandleUsePortal(portalPkt, player);

    EXPECT_EQ(player->GetRoom(), fieldChannel2) << "맵 간 이동은 떠난 채널 번호가 아니라 인원으로 채널을 고른다";
}

// 채널 변경은 퇴장과 입장을 거치는 룸 이동이다. 맵을 다시 열지 않으므로 맵 로드를 기다리지 않는다.
TEST_F(RoomChannelTest, ChangeChannelMovesPlayerToThatChannelRoom)
{
    shared_ptr<RecordingSession> watcherSession;
    CreatePlayerIn(townChannel1, MakePos(0.f, 500.f), watcherSession);
    shared_ptr<RecordingSession> residentSession;
    CreatePlayerIn(townChannel2, MakePos(0.f, -500.f), residentSession);
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayerIn(townChannel1, MakePos(200.f, 0.f), session);
    watcherSession->ClearSent();
    residentSession->ClearSent();

    townChannel1->C_HandleChangeChannel(MakeChangeChannel(2), player);

    const vector<Protocol::S_CHANGE_CHANNEL> responses = ChangeChannelResponses(session);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_SUCCESS);
    EXPECT_EQ(responses[0].channel(), 2);

    EXPECT_FALSE(townChannel1->Contains(player->GetEntityId()));
    EXPECT_TRUE(townChannel2->Contains(player->GetEntityId()));
    EXPECT_EQ(player->GetRoom(), townChannel2);
    EXPECT_EQ(player->GetMapId(), MAP_A);
    EXPECT_FLOAT_EQ(player->GetPosInfo().pos().x(), 200.f) << "채널을 바꿔도 같은 자리에 선다";

    const vector<Protocol::S_ENTER_ROOM> enterRooms = session->SentPackets<Protocol::S_ENTER_ROOM>(PKT_S_ENTER_ROOM);
    ASSERT_EQ(enterRooms.size(), 1u) << "클라이언트는 입장 응답을 받아야 앞 채널의 엔티티를 지운다";
    EXPECT_EQ(enterRooms[0].channel(), 2) << "클라이언트가 지금 채널을 표시한다";

    EXPECT_EQ(watcherSession->SentPackets<Protocol::S_DESPAWN>(PKT_S_DESPAWN).size(), 1u) << "떠난 채널에서는 사라진다";
    EXPECT_EQ(residentSession->SentPackets<Protocol::S_SPAWN>(PKT_S_SPAWN).size(), 1u) << "들어간 채널에서는 나타난다";
}

TEST_F(RoomChannelTest, EnterRoomCarriesChannel)
{
    shared_ptr<RecordingSession> session = make_shared<RecordingSession>();
    sessions.push_back(session);

    PlayerProgress progress;
    progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
    progress.level = 1;
    progress.hp = PLAYER_MAX_HP;
    PlayerSpawnParams params;
    params.session = session;
    params.progress = &progress;
    PlayerRef player = EntityFactory::Create<Player>(params);
    player->BeginMapLoad(townChannel2, MakePos(0.f, 0.f));
    townChannel2->C_HandleMapLoadComplete(player);

    const vector<Protocol::S_ENTER_ROOM> enterRooms = session->SentPackets<Protocol::S_ENTER_ROOM>(PKT_S_ENTER_ROOM);
    ASSERT_EQ(enterRooms.size(), 1u);
    EXPECT_EQ(enterRooms[0].channel(), 2);
}

TEST_F(RoomChannelTest, DeadPlayerCannotChangeChannel)
{
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayerIn(townChannel1, MakePos(0.f, 0.f), session);
    AttackInfo lethalAttack;
    lethalAttack.damage = PLAYER_MAX_HP * 10;
    player->OnHit(nullptr, lethalAttack);

    townChannel1->C_HandleChangeChannel(MakeChangeChannel(2), player);

    const vector<Protocol::S_CHANGE_CHANNEL> responses = ChangeChannelResponses(session);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_DEAD);
    EXPECT_TRUE(townChannel1->Contains(player->GetEntityId())) << "사망한 플레이어는 리스폰할 때까지 사망한 룸에 남는다";
}

TEST_F(RoomChannelTest, SameChannelIsRejected)
{
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayerIn(townChannel1, MakePos(0.f, 0.f), session);

    townChannel1->C_HandleChangeChannel(MakeChangeChannel(1), player);

    const vector<Protocol::S_CHANGE_CHANNEL> responses = ChangeChannelResponses(session);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_SAME_CHANNEL);
    EXPECT_TRUE(session->SentPackets<Protocol::S_ENTER_ROOM>(PKT_S_ENTER_ROOM).empty()) << "같은 룸에 다시 들어가지 않는다";
}

TEST_F(RoomChannelTest, PlayerLoadingMapCannotChangeChannel)
{
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayerIn(townChannel1, MakePos(1000.f, 0.f), session);
    Protocol::C_USE_PORTAL portalPkt;
    portalPkt.set_portal_id(PORTAL_TO_FIELD);
    townChannel1->C_HandleUsePortal(portalPkt, player);
    RoomRef loadingRoom = player->GetRoom();
    ASSERT_TRUE(player->IsLoadingMap());
    session->ClearSent();

    loadingRoom->C_HandleChangeChannel(MakeChangeChannel(loadingRoom->GetChannel() == 1 ? 2 : 1), player);

    const vector<Protocol::S_CHANGE_CHANNEL> responses = ChangeChannelResponses(session);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_MAP_LOADING);
    EXPECT_TRUE(player->IsLoadingMap()) << "거절해도 맵 로드 대기는 그대로다";
}

TEST_F(RoomChannelTest, ChannelOutsideMapIsRejected)
{
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayerIn(townChannel1, MakePos(0.f, 0.f), session);

    townChannel1->C_HandleChangeChannel(MakeChangeChannel(CHANNEL_COUNT + 1), player);

    const vector<Protocol::S_CHANGE_CHANNEL> responses = ChangeChannelResponses(session);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_INVALID_CHANNEL);
}

TEST_F(RoomChannelTest, ChannelListReportsPlayerCountPerChannel)
{
    shared_ptr<RecordingSession> session;
    PlayerRef player = CreatePlayerIn(townChannel1, MakePos(0.f, 0.f), session);
    shared_ptr<RecordingSession> otherSession;
    CreatePlayerIn(townChannel2, MakePos(0.f, 0.f), otherSession);
    CreatePlayerIn(townChannel2, MakePos(0.f, 0.f), otherSession);
    CreatePlayerIn(fieldChannel1, MakePos(7000.f, 0.f), otherSession);

    townChannel1->C_HandleChannelList(player);

    const vector<Protocol::S_CHANNEL_LIST> lists = session->SentPackets<Protocol::S_CHANNEL_LIST>(PKT_S_CHANNEL_LIST);
    ASSERT_EQ(lists.size(), 1u);
    ASSERT_EQ(lists[0].channels_size(), CHANNEL_COUNT) << "지금 맵의 채널만 싣는다";
    EXPECT_EQ(lists[0].channels(0).channel(), 1);
    EXPECT_EQ(lists[0].channels(0).player_count(), 1);
    EXPECT_EQ(lists[0].channels(1).channel(), 2);
    EXPECT_EQ(lists[0].channels(1).player_count(), 2);
}
