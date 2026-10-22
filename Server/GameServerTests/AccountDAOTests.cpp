#include "Core/pch.h"
#include <gtest/gtest.h>
#include "FakeDBConnection.h"
#include "Lobby/AccountDAO.h"
#include "DB/DAOCommon.h"
#include "Lobby/Data/LobbyData.h"

/*--------------------------------------------------------------
    계정 DAO 테스트

    로그인은 GameDB의 계정 행을 만들거나 마지막 로그인 시각을 갱신하고, 그 행의 캐릭터 슬롯 수를
    클라이언트에 알린다. 슬롯 수의 원천은 이 행이다. 새 행의 슬롯 수만 로비 기획 데이터의 기본값을 쓴다.

    픽스처 결합도: FakeDBConnection과 LobbyData::Install로 넣은 로비 표를 쓴다.
    행을 만드는지 갱신하는지는 실제 DB 테스트가 본다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 ACCOUNT_ID = 5'000'000'000;
    constexpr int32 DEFAULT_SLOT_COUNT = 3;
}

class AccountDAOTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        LobbyTables tables;
        tables.defaultCharacterSlotCount = DEFAULT_SLOT_COUNT;
        LobbyData::Install(std::move(tables));
    }

    void TearDown() override
    {
        LobbyData::Install(LobbyTables());
    }

    FakeDBConnection conn;
};

TEST_F(AccountDAOTest, BindsAccountAndDefaultSlotCount)
{
    conn.QueueResult({ { int64(DEFAULT_SLOT_COUNT) } });

    AccountDAO::RecordLogin(conn, ACCOUNT_ID);

    ASSERT_EQ(conn.executedParams.size(), 1u);
    EXPECT_EQ(get<int64>(conn.executedParams[0].at(0)), ACCOUNT_ID) << "int32로 바인딩하면 계정 번호가 잘린다";
    EXPECT_EQ(get<int64>(conn.executedParams[0].at(1)), DEFAULT_SLOT_COUNT) << "새 계정 행의 슬롯 수는 로비 표의 기본값이다";
}

TEST_F(AccountDAOTest, ReturnsSlotCountOfAccountRow)
{
    // 이미 있던 행은 기본값이 아니라 그 행의 값을 쓴다.
    conn.QueueResult({ { int64(6) } });

    EXPECT_EQ(AccountDAO::RecordLogin(conn, ACCOUNT_ID), 6);
}

TEST_F(AccountDAOTest, MissingResultRowIsDBError)
{
    conn.QueueResult({});

    EXPECT_THROW(AccountDAO::RecordLogin(conn, ACCOUNT_ID), DBError);
}

TEST_F(AccountDAOTest, FailedQueryIsDBError)
{
    conn.QueueExecuteFailure();

    EXPECT_THROW(AccountDAO::RecordLogin(conn, ACCOUNT_ID), DBError);
}
