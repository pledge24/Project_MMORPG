#include "Core/pch.h"
#include <gtest/gtest.h>
#include "FakeDBConnection.h"
#include "DB/CharacterStateDAO.h"
#include "DB/ProgressStorage.h"
#include "Network/GameEntry.h"
#include "Network/GameSessionManager.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerSaveData.h"
#include "Game/Inventory/InventoryComponent.h"

/*--------------------------------------------------------------
    진행 저장소와 DAO 테스트

    DAO는 연결을 인자로 받는다. 여기서는 ODBC 대신 FakeDBConnection을 넘겨, DAO가 바인딩한
    파라미터와 읽은 행을 확인한다. SQL 문장 자체가 맞는지는 이 테스트가 보지 않는다.

    저장 한 번은 트랜잭션 하나다. 중간 단계가 실패하면 앞 단계까지 되돌려야 레벨은 오르고 아이템은
    저장되지 않는 식으로 진행이 어긋나지 않는다.

    픽스처 결합도: FakeDBConnection만 쓴다. 입장 테스트는 소켓 없는 GameSession을 만든다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 USER_ID = 7;
    constexpr int64 CHARACTER_ID = 42;

    // 저장 DAO가 읽는 스탯이 모두 있는 사본. 바뀐 슬롯이 없으므로 아이템 저장은 쿼리를 실행하지 않는다.
    PlayerSaveData MakeSaveData()
    {
        PlayerSaveData data;
        data.userId = USER_ID;
        data.progress.playerInfo.set_character_id(CHARACTER_ID);
        data.progress.playerInfo.set_level(3);

        auto* stats = data.progress.statInfo.mutable_info();
        for (Protocol::StatType statType : { Protocol::STAT_TYPE_EXP, Protocol::STAT_TYPE_HP, Protocol::STAT_TYPE_MP,
            Protocol::STAT_TYPE_PHYSICAL_ATTACK, Protocol::STAT_TYPE_MAGICAL_ATTACK })
            (*stats)[statType] = 10;

        data.gearDirtyFlags = vector<bool>(MAX_SLOTS, false);
        data.consumableDirtyFlags = vector<bool>(MAX_SLOTS, false);
        data.miscDirtyFlags = vector<bool>(MAX_SLOTS, false);
        return data;
    }
}

/* 불러오기 */

TEST(ProgressStorageTest, LoadCharacterReadsRowOfThatAccount)
{
    FakeDBConnection conn;
    conn.QueueResult({ { int64(Protocol::CLASS_TYPE_WARRIOR), wstring(L"전사"), int64(3) } });

    PlayerProgress progress;
    ASSERT_TRUE(CharacterStateDAO::LoadCharacter(conn, USER_ID, CHARACTER_ID, OUT progress));

    EXPECT_EQ(progress.playerInfo.character_id(), CHARACTER_ID);
    EXPECT_EQ(progress.playerInfo.class_(), Protocol::CLASS_TYPE_WARRIOR);
    EXPECT_EQ(progress.playerInfo.level(), 3);
    EXPECT_FALSE(progress.playerInfo.name().empty());

    ASSERT_EQ(conn.executedParams.size(), 1u);
    EXPECT_EQ(get<int64>(conn.executedParams[0].at(0)), CHARACTER_ID);
    EXPECT_EQ(get<int64>(conn.executedParams[0].at(1)), USER_ID) << "클라이언트가 보낸 캐릭터 번호를 믿지 않고 계정과 함께 대조한다";
}

TEST(ProgressStorageTest, LoadCharacterOfOtherAccountFails)
{
    FakeDBConnection conn;
    conn.QueueResult({});

    PlayerProgress progress;
    EXPECT_FALSE(CharacterStateDAO::LoadCharacter(conn, USER_ID, CHARACTER_ID, OUT progress)) << "행이 없으면 다른 계정의 캐릭터다";
}

TEST(ProgressStorageTest, FailedLoadLeavesSessionWithoutPlayer)
{
    FakeDBConnection conn;
    conn.QueueExecuteFailure();

    GameSessionRef session = make_shared<GameSession>();
    GameSessionManager sessionManager;
    sessionManager.RegisterUser(USER_ID, session);

    const Protocol::S_ENTER_GAME pkt = GameEntry::Enter(conn, session, CHARACTER_ID);

    EXPECT_FALSE(pkt.success());
    EXPECT_EQ(session->GetPlayer(), nullptr) << "불러오기에 실패한 세션에 플레이어가 남으면 끊길 때 반쯤 채운 진행을 저장한다";
}

/* 저장 */

TEST(ProgressStorageTest, SaveCommitsOneTransaction)
{
    FakeDBConnection conn;

    ProgressStorage::Save(conn, MakeSaveData());

    EXPECT_EQ(conn.beginCount, 1);
    EXPECT_EQ(conn.commitCount, 1);
    EXPECT_EQ(conn.rollbackCount, 0);
}

TEST(ProgressStorageTest, UnexpectedExceptionRollsBack)
{
    FakeDBConnection conn;
    conn.QueueResult({});
    conn.QueueExecuteException();

    EXPECT_FALSE(ProgressStorage::Save(conn, MakeSaveData()));
    EXPECT_EQ(conn.rollbackCount, 1) << "열린 트랜잭션을 둔 채 연결을 풀에 돌려주면 다음 잡이 그 위에서 돈다";
}

// TD-032: 아이템 저장이 실패하면 앞서 실행한 레벨과 마지막 상태의 갱신도 되돌린다.
TEST(ProgressStorageTest, FailedItemSaveRollsBackEarlierSteps)
{
    FakeDBConnection conn;
    PlayerSaveData data = MakeSaveData();
    data.gearDirtyFlags.reset();

    ProgressStorage::Save(conn, data);

    EXPECT_EQ(conn.executedQueries.size(), 2u) << "레벨과 마지막 상태는 이미 실행됐다";
    EXPECT_EQ(conn.commitCount, 0);
    EXPECT_EQ(conn.rollbackCount, 1) << "되돌리지 않으면 레벨은 오르고 아이템은 저장되지 않은 채로 남는다";
}
