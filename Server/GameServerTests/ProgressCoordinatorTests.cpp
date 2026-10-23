#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Network/ProgressCoordinator.h"
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "PlayerTestAccess.h"

/*--------------------------------------------------------------
    진행 조율자 테스트

    접속 종료 저장이 끝나기 전에는 같은 계정의 입장 불러오기를 하지 않는다.
    저장 잡은 룸 큐를 거쳐 늦게 DB 큐에 들어가므로, 새 세션의 불러오기가 먼저 돌면 저장 전의 진행을
    불러오고 새 세션이 끊길 때 그 낡은 진행으로 덮어쓴다.

    픽스처 결합도: 지역 SaveGate를 주입한 조율자를 전역 GProgressCoordinator에 끼운다.
    DB 잡과 만료 타이머는 목록에 쌓아 두고 테스트가 순서를 정해 돌린다.
    저장과 불러오기는 일어난 순서만 기록한다. 룸은 Room::Create로 만들고 Start()는 부르지 않는다. 룸 큐의 잡은
    테스트 스레드에서 곧바로 돈다. 플레이어는 세션 없이 불러온 진행으로 만든다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 USER_ID = 7;
    constexpr int32 ROOM_ID = 10;
}

class ProgressCoordinatorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 룸 이동 중에 끊긴 플레이어는 Room::EnterPlayer가 전역 조율자로 넘긴다. 그 경로도 이 조율자로 받는다.
        previousCoordinator = GProgressCoordinator;
        GProgressCoordinator = &coordinator;

        LevelTemplate level1;
        level1.level = 1;
        level1.maxHp = 100;

        GamedataTables tables;
        tables.classLevelTables[Protocol::CHARACTER_CLASS_WARRIOR] = ClassLevelTable({ level1 });
        Gamedata::Install(std::move(tables));

        MapTemplate mapTemplate;
        mapTemplate.templateId = ROOM_ID;
        mapTemplate.depthHalfExtent = 5000.f;
        mapTemplate.widthHalfExtent = 5000.f;
        room = Room::Create(mapTemplate);
        ASSERT_NE(room, nullptr);

        player = CreatePlayer(USER_ID);
        ASSERT_NE(player, nullptr);
    }

    /** ROOM_ID에서 불러온 1레벨 전사. 세션은 없다. */
    static PlayerRef CreatePlayer(int64 userId)
    {
        PlayerProgress progress;
        progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
        progress.level = 1;

        PlayerSpawnParams params;
        params.userId = userId;
        params.progress = &progress;
        return EntityFactory::Create<Player>(params);
    }

    /** ROOM_ID + 1번 룸. 룸 이동의 목적지로 쓴다. */
    static RoomRef CreateNextRoom()
    {
        MapTemplate nextMap;
        nextMap.templateId = ROOM_ID + 1;
        nextMap.depthHalfExtent = 5000.f;
        nextMap.widthHalfExtent = 5000.f;
        return Room::Create(nextMap);
    }

    void TearDown() override
    {
        player.reset();
        room.reset();
        Gamedata::Install(GamedataTables());
        GProgressCoordinator = previousCoordinator;
    }

    void EnterRoom()
    {
        RoomEnterData enterData{};
        enterData.nextRoomId = ROOM_ID;
        room->EnterPlayer(player, enterData);
    }

    /** 로비 복귀. 저장 뒤의 일은 목록을 보낸 것으로 기록한다. */
    void ReturnToLobby()
    {
        coordinator.OnReturnToLobby(USER_ID, player, [this]() { events.push_back("list"); });
    }

    void RequestEnter()
    {
        coordinator.RequestEnter(USER_ID, {
            [this]() { events.push_back("load"); },
            [this]() { events.push_back("reject"); } });
    }

    /** 쌓인 DB 잡을 넣은 순서대로 돌린다. 도는 중에 들어온 잡도 돌린다. */
    void RunDBJobs()
    {
        while (dbJobs.empty() == false)
        {
            CallbackType job = std::move(dbJobs.front());
            dbJobs.pop_front();
            job();
        }
    }

    /** 예약된 만료 타이머를 모두 돌린다. */
    void FireTimers()
    {
        vector<CallbackType> fired = std::move(timers);
        timers.clear();
        for (CallbackType& timer : fired)
            timer();
    }

    SaveGate gate;
    deque<CallbackType> dbJobs;
    vector<CallbackType> timers;
    vector<string> events;
    vector<PlayerSaveData> savedData;
    bool failSave = false;

    ProgressCoordinator coordinator{
        gate,
        [this](int64 userId, CallbackType job) { dbJobs.push_back(std::move(job)); },
        [this](const PlayerSaveData& data)
        {
            events.push_back("save");
            savedData.push_back(data);
            if (failSave)
                throw runtime_error("저장 실패");
        },
        [this](uint64 delayMs, CallbackType job) { timers.push_back(std::move(job)); } };

    ProgressCoordinator* previousCoordinator = nullptr;
    RoomRef room;
    PlayerRef player;
};

TEST_F(ProgressCoordinatorTest, EnterWithoutPendingSaveLoadsAtOnce)
{
    RequestEnter();
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "load" }));
    EXPECT_TRUE(timers.empty()) << "기다리지 않는 입장에는 만료 타이머를 걸지 않는다";
}

// 접속 종료 중 재입장: 끊긴 세션의 저장이 DB 큐에 들어간 뒤에 온 입장은 그 저장이 끝난 뒤에 불러온다.
TEST_F(ProgressCoordinatorTest, ReenterWhileSavingLoadsAfterSave)
{
    EnterRoom();

    coordinator.OnDisconnected(USER_ID, player);
    RequestEnter();
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "save", "load" }));
    EXPECT_FALSE(room->Contains(player->GetEntityId()));
}

// 같은 계정의 새 로비 접속: 밀려난 세션이 아직 끊기지 않았어도 새 세션의 입장은 그 세션의 저장을 기다린다.
TEST_F(ProgressCoordinatorTest, DuplicateConnectionWaitsForReplacedSessionSave)
{
    EnterRoom();

    coordinator.OnDuplicateConnection(USER_ID, player);
    RequestEnter();
    RunDBJobs();
    EXPECT_TRUE(events.empty()) << "밀려난 세션이 끊기기 전에는 불러오지 않는다";

    coordinator.OnDisconnected(USER_ID, player);
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "save", "load" }));
}

// 대기 만료: 상한 안에 저장이 끝나지 않으면 입장을 거절하고 불러오지 않는다.
TEST_F(ProgressCoordinatorTest, ExpiredWaitRejectsEnter)
{
    EnterRoom();
    coordinator.OnDuplicateConnection(USER_ID, player);

    RequestEnter();
    ASSERT_EQ(timers.size(), 1u);
    FireTimers();

    EXPECT_EQ(events, vector<string>({ "reject" }));
}

TEST_F(ProgressCoordinatorTest, SecondEnterWhileWaitingIsRejected)
{
    EnterRoom();
    coordinator.OnDuplicateConnection(USER_ID, player);

    RequestEnter();
    RequestEnter();
    EXPECT_EQ(events, vector<string>({ "reject" }));

    coordinator.OnDisconnected(USER_ID, player);
    RunDBJobs();
    EXPECT_EQ(events, vector<string>({ "reject", "save", "load" })) << "먼저 맡긴 입장은 저장이 끝난 뒤에 불러온다";
}

TEST_F(ProgressCoordinatorTest, FailedSaveStillLetsEnterLoad)
{
    EnterRoom();
    failSave = true;

    coordinator.OnDisconnected(USER_ID, player);
    RequestEnter();
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "save", "load" })) << "실패한 저장은 다시 시도하지 않으므로 기다려도 결과가 같다";
}

// 룸에 들어간 적이 없는 플레이어는 이 세션에서 바뀐 것이 없으므로 저장하지 않고, 다음 입장을 막지 않는다.
TEST_F(ProgressCoordinatorTest, PlayerNeverInRoomIsNotSaved)
{
    coordinator.OnDisconnected(USER_ID, player);
    RequestEnter();
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "load" }));
}

// 룸 이동 중에 끊긴 플레이어는 떠난 룸이 찾지 못하므로 들어갈 룸이 퇴장과 저장을 이어 받는다.
TEST_F(ProgressCoordinatorTest, DisconnectDuringRoomTransferSavesInNextRoom)
{
    EnterRoom();
    RoomRef nextRoom = CreateNextRoom();
    ASSERT_TRUE(room->LeavePlayer(player, true));

    coordinator.OnDisconnected(USER_ID, player);
    RequestEnter();
    RunDBJobs();
    EXPECT_TRUE(events.empty()) << "떠난 룸은 저장하지 않지만 대기는 남는다";

    RoomEnterData enterData{};
    enterData.nextRoomId = ROOM_ID + 1;
    EXPECT_FALSE(nextRoom->EnterPlayer(player, enterData));
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "save", "load" }));
    EXPECT_FALSE(nextRoom->Contains(player->GetEntityId()));
}

// 룸 이동 중에 끊긴 플레이어가 들어갈 룸에 들어가지 못하면, 떠난 룸도 들어갈 룸도 저장하지 않아 대기가 풀리지 않았다.
// 들어갈 룸에 같은 엔티티 번호가 이미 있으면 들어가지 못한다.
TEST_F(ProgressCoordinatorTest, DisconnectDuringFailedRoomTransferStillSaves)
{
    EnterRoom();
    RoomRef nextRoom = CreateNextRoom();
    PlayerRef occupant = CreatePlayer(USER_ID + 1);
    EntityTestAccess::SetEntityId(*occupant, player->GetEntityId());
    RoomEnterData occupantEnter{};
    occupantEnter.nextRoomId = ROOM_ID + 1;
    ASSERT_TRUE(nextRoom->EnterPlayer(occupant, occupantEnter));
    ASSERT_TRUE(room->LeavePlayer(player, true));

    coordinator.OnDisconnected(USER_ID, player);
    RequestEnter();
    RunDBJobs();

    RoomEnterData enterData{};
    enterData.nextRoomId = ROOM_ID + 1;
    EXPECT_FALSE(nextRoom->EnterPlayer(player, enterData));
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "save", "load" })) << "어느 룸에도 없는 플레이어도 저장하고 대기를 푼다";
}

// 다른 맵으로 가 맵 로드를 기다리는 플레이어는 어느 룸에도 없다. 룸에 있는지로만 저장을 정하면 아무도 저장하지 않아
// 대기가 풀리지 않는다. 기다리는 룸이 소속 룸이므로 그 룸 큐가 저장한다.
TEST_F(ProgressCoordinatorTest, DisconnectWhileWaitingForMapLoadSavesInWaitingRoom)
{
    EnterRoom();
    RoomRef nextRoom = CreateNextRoom();
    ASSERT_TRUE(room->LeavePlayer(player, true));
    player->BeginMapLoad(nextRoom, player->GetPosInfo());

    coordinator.OnDisconnected(USER_ID, player);
    RequestEnter();
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "save", "load" }));

    nextRoom->C_HandleMapLoadComplete(player);
    EXPECT_FALSE(nextRoom->Contains(player->GetEntityId())) << "저장한 플레이어의 늦은 로드 완료는 버린다";
}

// 접속 종료가 소속 룸을 읽은 직후에 플레이어가 다른 맵으로 떠나면, 떠난 룸 큐에 넣은 저장 잡은 플레이어를 찾지 못한다.
// 소속 룸이 바뀌었으므로 그 룸 큐로 넘겨 저장한다.
TEST_F(ProgressCoordinatorTest, DisconnectReadBeforeMapLoadIsForwardedToWaitingRoom)
{
    EnterRoom();
    RoomRef nextRoom = CreateNextRoom();
    ASSERT_TRUE(player->MarkDisconnected());
    ASSERT_TRUE(room->LeavePlayer(player, true));
    player->BeginMapLoad(nextRoom, player->GetPosInfo());

    coordinator.LeaveRoomAndSave(room, player); // 떠난 룸 큐에 넣었던 저장 잡
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "save" }));
}

// 룸 입장 잡이 큐에 있는 동안 끊기면 대기 없이 새 세션이 불러오고, 늦게 돈 룸 입장 잡이 그 뒤에 저장을 넣었다.
// 룸에 들어간 적이 없으므로 저장할 것이 없다.
TEST_F(ProgressCoordinatorTest, DisconnectBeforeFirstRoomEntryIsNotSavedAfterReload)
{
    coordinator.OnDisconnected(USER_ID, player);
    RequestEnter();
    EnterRoom(); // 큐에 남아 있던 룸 입장 잡
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "load" }));
    EXPECT_FALSE(room->Contains(player->GetEntityId())) << "끊긴 플레이어는 룸에 남지 않는다";
}

// 상한이 지나 입장을 거절하면서 대기까지 지웠다. 그 뒤의 입장은 늦게 도착한 저장보다 먼저 불러왔다.
TEST_F(ProgressCoordinatorTest, EnterAfterExpiryStillWaitsForSave)
{
    EnterRoom();
    coordinator.OnDuplicateConnection(USER_ID, player);
    RequestEnter();
    FireTimers();

    RequestEnter();
    RunDBJobs();
    EXPECT_EQ(events, vector<string>({ "reject" })) << "만료가 저장을 끝내지는 않는다";

    coordinator.OnDisconnected(USER_ID, player);
    RunDBJobs();
    EXPECT_EQ(events, vector<string>({ "reject", "save", "load" }));
}

// 끊긴 세션이 계정 등록에서 빠지기 전에 새 로비 접속이 그 세션을 밀어낼 수 있다. 그 접속 종료가 이미 대기를 풀었으므로
// 밀어내기가 건 대기를 풀어 줄 곳이 없어, 다음 입장이 영영 기다렸다.
TEST_F(ProgressCoordinatorTest, LateDuplicateConnectionAfterDisconnectBeforeRoomDoesNotBlockEnter)
{
    coordinator.OnDisconnected(USER_ID, player);
    coordinator.OnDuplicateConnection(USER_ID, player);

    RequestEnter();
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "load" }));
}

TEST_F(ProgressCoordinatorTest, LateDuplicateConnectionAfterSaveDoesNotBlockEnter)
{
    EnterRoom();
    coordinator.OnDisconnected(USER_ID, player);
    RunDBJobs();
    coordinator.OnDuplicateConnection(USER_ID, player);

    RequestEnter();
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "save", "load" }));
}

// 저장이 아직 끝나지 않았으면 늦게 온 밀어내기가 있어도 새 입장은 그 저장을 기다린다.
TEST_F(ProgressCoordinatorTest, LateDuplicateConnectionBeforeSaveStillWaitsForSave)
{
    EnterRoom();
    coordinator.OnDisconnected(USER_ID, player);
    coordinator.OnDuplicateConnection(USER_ID, player);

    RequestEnter();
    EXPECT_TRUE(events.empty());
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "save", "load" }));
}

// 로비 복귀: 곧바로 룸에서 퇴장하고, 캐릭터 목록은 저장이 끝난 뒤에 보낸다. 먼저 보내면 저장 전의 레벨이 보인다.
TEST_F(ProgressCoordinatorTest, ReturnToLobbySendsListAfterSave)
{
    EnterRoom();

    ReturnToLobby();
    EXPECT_FALSE(room->Contains(player->GetEntityId())) << "복귀를 받으면 곧바로 룸에서 퇴장한다";
    EXPECT_TRUE(events.empty()) << "저장 잡이 돌기 전에는 목록을 보내지 않는다";

    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "save", "list" }));
}

TEST_F(ProgressCoordinatorTest, ReturnToLobbyBeforeFirstRoomEntrySendsListWithoutSave)
{
    ReturnToLobby();
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "list" })) << "룸에 들어간 적이 없으면 바뀐 진행이 없어 저장하지 않는다";
}

// 복귀 중에 연결이 끊기면 접속 종료도 같은 플레이어를 저장하려 한다. 두 번 저장하면 앞의 저장 잡이 대기를 푼 뒤에
// 뒤의 저장이 돌아, 그사이 입장한 새 세션의 불러오기 뒤에 낡은 저장이 덮인다.
TEST_F(ProgressCoordinatorTest, DisconnectDuringReturnSavesOnce)
{
    EnterRoom();

    ReturnToLobby();
    coordinator.OnDisconnected(USER_ID, player);
    RunDBJobs();

    EXPECT_EQ(ranges::count(events, string("save")), 1);
}

// 복귀의 저장이 끝난 뒤에 접속 종료가 같은 플레이어로 와도 대기를 남기지 않는다. 남기면 다음 입장이 상한까지 막힌다.
TEST_F(ProgressCoordinatorTest, DisconnectAfterReturnSaveDoesNotBlockEnter)
{
    EnterRoom();
    ReturnToLobby();
    RunDBJobs();

    coordinator.OnDisconnected(USER_ID, player);
    RunDBJobs();
    events.clear();

    RequestEnter();
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "load" }));
    EXPECT_TRUE(timers.empty()) << "기다릴 저장이 없으면 만료 타이머를 걸지 않는다";
}

// 복귀한 뒤 다시 입장하면 복귀가 저장한 진행을 불러온다. 같은 계정의 입장은 그 저장이 끝나야 불러온다.
TEST_F(ProgressCoordinatorTest, ReenterAfterReturnLoadsAfterSave)
{
    EnterRoom();
    ReturnToLobby();
    RunDBJobs();

    RequestEnter();
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "save", "list", "load" }));
}

// 사망한 채 복귀하면 접속 종료와 같이 마을로 리스폰한 상태로 저장한다.
TEST_F(ProgressCoordinatorTest, ReturnWhileDeadSavesTownRespawn)
{
    constexpr int32 TOWN_ROOM_ID = 1;
    constexpr int32 TOWN_MAP_ID = 1001;

    LevelTemplate level1;
    level1.level = 1;
    level1.maxHp = 100;

    MapTemplate town;
    town.templateId = TOWN_ROOM_ID;
    town.mapId = TOWN_MAP_ID;
    town.respawnPoint = TemplatePos{ 300.f, 0.f, 0.f };

    GamedataTables tables;
    tables.classLevelTables[Protocol::CHARACTER_CLASS_WARRIOR] = ClassLevelTable({ level1 });
    tables.maps[TOWN_ROOM_ID] = town;
    tables.townRoomId = TOWN_ROOM_ID;
    Gamedata::Install(std::move(tables));

    EnterRoom();
    AttackInfo lethalAttack;
    lethalAttack.damage = 1000;
    player->OnHit(nullptr, lethalAttack);
    ASSERT_TRUE(player->IsDead());

    ReturnToLobby();
    RunDBJobs();

    ASSERT_EQ(savedData.size(), 1u);
    EXPECT_EQ(savedData[0].progress.mapId, TOWN_MAP_ID);
    EXPECT_EQ(savedData[0].progress.hp, 50) << "마을 리스폰은 HP를 절반으로 채운다";
}
