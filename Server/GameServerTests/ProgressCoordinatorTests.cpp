#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Network/ProgressCoordinator.h"
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"

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
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1 });
        Gamedata::Install(std::move(tables));

        MapTemplate mapTemplate;
        mapTemplate.templateId = ROOM_ID;
        mapTemplate.depthHalfExtent = 5000.f;
        mapTemplate.widthHalfExtent = 5000.f;
        room = Room::Create(mapTemplate);
        ASSERT_NE(room, nullptr);

        PlayerProgress progress;
        progress.playerInfo.set_class_(Protocol::CLASS_TYPE_WARRIOR);
        progress.playerInfo.set_level(1);
        progress.playerInfo.set_room_id(ROOM_ID);

        auto* stats = progress.statInfo.mutable_info();
        for (Protocol::StatType statType : { Protocol::STAT_TYPE_EXP, Protocol::STAT_TYPE_HP, Protocol::STAT_TYPE_MP,
            Protocol::STAT_TYPE_PHYSICAL_ATTACK, Protocol::STAT_TYPE_MAGICAL_ATTACK })
            (*stats)[statType] = 0;

        PlayerSpawnParams params;
        params.userId = USER_ID;
        params.progress = &progress;
        player = EntityFactory::Create<Player>(params);
        ASSERT_NE(player, nullptr);
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
        enterData.enterType = Protocol::ENTER_TYPE_INITIAL;
        room->EnterPlayer(player, enterData);
    }

    void RequestEnter()
    {
        coordinator.RequestEnter(USER_ID,
            [this]() { events.push_back("load"); },
            [this]() { events.push_back("reject"); });
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
    bool failSave = false;

    ProgressCoordinator coordinator{
        gate,
        [this](int64 userId, CallbackType job) { dbJobs.push_back(std::move(job)); },
        [this](const PlayerSaveData& data)
        {
            events.push_back("save");
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

// 중복 로그인: 밀려난 세션이 아직 끊기지 않았어도 새 세션의 입장은 그 세션의 저장을 기다린다.
TEST_F(ProgressCoordinatorTest, DuplicateLoginWaitsForReplacedSessionSave)
{
    EnterRoom();

    coordinator.OnDuplicateLogin(USER_ID, player);
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
    coordinator.OnDuplicateLogin(USER_ID, player);

    RequestEnter();
    ASSERT_EQ(timers.size(), 1u);
    FireTimers();

    EXPECT_EQ(events, vector<string>({ "reject" }));
}

TEST_F(ProgressCoordinatorTest, SecondEnterWhileWaitingIsRejected)
{
    EnterRoom();
    coordinator.OnDuplicateLogin(USER_ID, player);

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
    MapTemplate nextMap;
    nextMap.templateId = ROOM_ID + 1;
    nextMap.depthHalfExtent = 5000.f;
    nextMap.widthHalfExtent = 5000.f;
    RoomRef nextRoom = Room::Create(nextMap);
    ASSERT_NE(nextRoom, nullptr);
    ASSERT_TRUE(room->LeavePlayer(player, true));

    coordinator.OnDisconnected(USER_ID, player);
    RequestEnter();
    RunDBJobs();
    EXPECT_TRUE(events.empty()) << "떠난 룸은 저장하지 않지만 대기는 남는다";

    RoomEnterData enterData{};
    enterData.nextRoomId = ROOM_ID + 1;
    enterData.enterType = Protocol::ENTER_TYPE_SAME_MAP_TRANSFER;
    EXPECT_FALSE(nextRoom->EnterPlayer(player, enterData));
    RunDBJobs();

    EXPECT_EQ(events, vector<string>({ "save", "load" }));
    EXPECT_FALSE(nextRoom->Contains(player->GetEntityId()));
}

// TD-003: 룸 입장 잡이 큐에 있는 동안 끊기면 대기 없이 새 세션이 불러오고, 늦게 돈 룸 입장 잡이 그 뒤에 저장을 넣었다.
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

// TD-003: 상한이 지나 입장을 거절하면서 대기까지 지웠다. 그 뒤의 입장은 늦게 도착한 저장보다 먼저 불러왔다.
TEST_F(ProgressCoordinatorTest, EnterAfterExpiryStillWaitsForSave)
{
    EnterRoom();
    coordinator.OnDuplicateLogin(USER_ID, player);
    RequestEnter();
    FireTimers();

    RequestEnter();
    RunDBJobs();
    EXPECT_EQ(events, vector<string>({ "reject" })) << "만료가 저장을 끝내지는 않는다";

    coordinator.OnDisconnected(USER_ID, player);
    RunDBJobs();
    EXPECT_EQ(events, vector<string>({ "reject", "save", "load" }));
}
