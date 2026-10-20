#include "Core/pch.h"
#include <gtest/gtest.h>
#include "FakeDBConnection.h"
#include "DB/IssuedIdDAO.h"
#include "DB/DAOCommon.h"
#include "Utils/SnowflakeIdGenerator.h"

/*--------------------------------------------------------------
    발급한 ID 조회 DAO 테스트

    부팅할 때 이 노드가 발급한 캐릭터 ID와 아이템 UID의 최댓값을 읽어 발급기의 시작점으로 쓴다.
    최댓값을 int32로 읽으면 Snowflake ID가 잘려, 새로 발급한 ID가 기존 행과 겹친다.

    픽스처 결합도: FakeDBConnection만 쓴다. 노드로 거르는 SQL은 실제 DB 테스트가 본다.
---------------------------------------------------------------*/

TEST(IssuedIdDAOTest, BindsNodeAndBitLayout)
{
    FakeDBConnection conn;
    conn.QueueResult({ { int64(0) } });

    IssuedIdDAO::GetLastIssuedId(conn, 1023);

    ASSERT_EQ(conn.executedParams.size(), 1u);
    const map<int32, FakeDBValue>& params = conn.executedParams[0];
    EXPECT_EQ(get<int64>(params.at(0)), int64{ 1 } << SnowflakeIdGenerator::SEQUENCE_BITS) << "노드 번호는 순번 비트 위에 있다";
    EXPECT_EQ(get<int64>(params.at(1)), int64{ SnowflakeIdGenerator::MAX_NODE_ID } + 1);
    EXPECT_EQ(get<int64>(params.at(2)), 1023);
}

TEST(IssuedIdDAOTest, LastIssuedIdBeyondInt32IsReadWhole)
{
    constexpr int64 LAST_ISSUED_ID = 5'000'000'000'000;
    FakeDBConnection conn;
    conn.QueueResult({ { LAST_ISSUED_ID } });

    EXPECT_EQ(IssuedIdDAO::GetLastIssuedId(conn, 0), LAST_ISSUED_ID) << "최댓값이 잘리면 새 ID가 기존 행과 겹친다";
}

TEST(IssuedIdDAOTest, MissingResultRowIsDBError)
{
    FakeDBConnection conn;
    conn.QueueResult({});

    EXPECT_THROW(IssuedIdDAO::GetLastIssuedId(conn, 0), DBError) << "발급한 적이 없어도 쿼리는 0을 돌려준다. 행이 없으면 쿼리가 잘못된 것이다";
}
