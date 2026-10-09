#include "Core/pch.h"
#include <gtest/gtest.h>
#include "FakeDBConnection.h"
#include "DB/DAOCommon.h"

/*--------------------------------------------------------------
    배열 파라미터 저장 테스트

    아이템 저장은 여러 행을 배열 파라미터로 묶어 쿼리 한 번에 보낸다. 바인딩 배열은 MAX_PARAM_ROWS칸이라
    그보다 많은 행을 받으면 배열 밖에 쓴다. 넘으면 실행하지 않고 DBError로 알려야 한다.

    픽스처 결합도: FakeDBConnection만 쓴다.
---------------------------------------------------------------*/

namespace
{
    struct ValueRowsBinding
    {
        static constexpr int32 PARAMS = 1;

        ValueRowsBinding(DBBind<PARAMS, 0>& dbBind, const vector<int64>& rows)
        {
            for (size_t i = 0; i < rows.size(); i++)
                _values[i] = rows[i];

            dbBind.BindParamSet(0, _values, static_cast<int32>(rows.size()));
        }

        int64 _values[MAX_PARAM_ROWS] = {};
    };

    const WCHAR* QUERY = L"INSERT INTO T VALUES (?)";
}

TEST(ParamSetTest, EmptyRowsAreNotExecuted)
{
    FakeDBConnection conn;

    ExecuteParamSet<ValueRowsBinding>(conn, "Test", QUERY, vector<int64>());

    EXPECT_TRUE(conn.executedQueries.empty()) << "바뀐 행이 없는 저장은 쿼리를 보내지 않는다";
}

TEST(ParamSetTest, RowsAreSentInOneExecution)
{
    FakeDBConnection conn;

    ExecuteParamSet<ValueRowsBinding>(conn, "Test", QUERY, vector<int64>{ 11, 22, 33 });

    ASSERT_EQ(conn.executedQueries.size(), 1u);
    EXPECT_EQ(get<int64>(conn.executedParams[0].at(0)), 11);
}

TEST(ParamSetTest, RowsOverLimitAreRejectedWithoutExecuting)
{
    FakeDBConnection conn;
    const vector<int64> rows(MAX_PARAM_ROWS + 1, 7);

    EXPECT_THROW(ExecuteParamSet<ValueRowsBinding>(conn, "Test", QUERY, rows), DBError);
    EXPECT_TRUE(conn.executedQueries.empty());
}

// TD-040: ODBC Driver 17은 일부 행만 실패해도 SQL_SUCCESS_WITH_INFO를 돌려주고 나머지 행을 저장한다.
// 2026년 10월 9일 DriverReproTest로 실제 DB에서 확인했다. 실행 결과만 보면 아이템 일부가 사라진 저장이 성공으로 끝난다.
TEST(ParamSetTest, PartiallyFailedRowsAreDBError)
{
    FakeDBConnection conn;
    conn.QueuePartialParamSetFailure({ 1 });

    EXPECT_THROW(ExecuteParamSet<ValueRowsBinding>(conn, "Test", QUERY, vector<int64>{ 11, 22, 33 }), DBError)
        << "실패한 행을 놓치면 저장 트랜잭션이 되돌려지지 않고 그 행의 아이템이 사라진다";
}

TEST(ParamSetTest, FailedExecutionIsDBError)
{
    FakeDBConnection conn;
    conn.QueueExecuteFailure();

    EXPECT_THROW(ExecuteParamSet<ValueRowsBinding>(conn, "Test", QUERY, vector<int64>{ 1 }), DBError);
}
