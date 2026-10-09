#include "Core/pch.h"
#include <gtest/gtest.h>
#include <sql.h>
#include <sqlext.h>
#include "Core/Config.h"

/*--------------------------------------------------------------
    ODBC 드라이버 동작 재현 (TD-040)

    배열 파라미터로 여러 행을 한 번에 실행할 때, 일부 행만 실패하면 SQLExecDirect가 무엇을 돌려주는지
    확인한다. ODBC 명세는 SQL_SUCCESS_WITH_INFO를 돌려줄 수 있다고 하고, DBConnection::Execute는 그 값을
    성공으로 본다. 드라이버가 실제로 그렇게 하는지는 실제 DB에서만 알 수 있다.

    기본으로 꺼져 있다. 사람이 실제 DB를 띄운 뒤 아래처럼 돌리고 출력을 확인한다. 쓰는 것은 연결이 끝나면
    사라지는 세션 임시 테이블(#)뿐이다. 접속 문자열은 게임 서버와 같다(P1_GAME_DB_CONNECTION_STRING 또는 기본값).

        GameServerTests.exe --gtest_also_run_disabled_tests --gtest_filter=DriverReproTest.*

    2026년 10월 9일 결과(ODBC Driver 17 for SQL Server, LocalDB): 두 경우 모두 SQLExecDirect가 SQL_SUCCESS_WITH_INFO를
    돌려주고, 처리한 행은 3/3, 두 번째 행의 상태만 SQL_PARAM_ERROR, 대상 테이블에는 두 행이 남았다. 그래서
    ExecuteParamSet(DB/DAOCommon.h)이 행별 상태를 대조한다. 드라이버를 바꾸면 다시 돌려 본다.

    픽스처 결합도: 실제 DB가 필요하다. 결과를 판정하지 않고 출력만 한다.
---------------------------------------------------------------*/

namespace
{
    constexpr SQLULEN ROW_COUNT = 3;

    /** 연결 하나와 statement 하나. 테스트가 끝나면 해제한다. */
    struct ReproConnection
    {
        SQLHENV env = SQL_NULL_HANDLE;
        SQLHDBC dbc = SQL_NULL_HANDLE;
        SQLHSTMT stmt = SQL_NULL_HANDLE;

        bool Open()
        {
            const Config config = Config::Load(&Config::ReadProcessEnv);

            ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
            ::SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
            ::SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

            wstring connectionString = config.dbConnectionString;
            SQLRETURN ret = ::SQLDriverConnectW(dbc, NULL, connectionString.data(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
            if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
                return false;

            return ::SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt) == SQL_SUCCESS;
        }

        ~ReproConnection()
        {
            if (stmt != SQL_NULL_HANDLE) ::SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            if (dbc != SQL_NULL_HANDLE) { ::SQLDisconnect(dbc); ::SQLFreeHandle(SQL_HANDLE_DBC, dbc); }
            if (env != SQL_NULL_HANDLE) ::SQLFreeHandle(SQL_HANDLE_ENV, env);
        }

        SQLRETURN Exec(const WCHAR* query)
        {
            ::SQLFreeStmt(stmt, SQL_CLOSE);
            return ::SQLExecDirectW(stmt, const_cast<SQLWCHAR*>(query), SQL_NTS);
        }

        int64 CountRows(const WCHAR* table)
        {
            ::SQLFreeStmt(stmt, SQL_UNBIND);
            ::SQLFreeStmt(stmt, SQL_RESET_PARAMS);
            ::SQLSetStmtAttr(stmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)1, 0);
            ::SQLSetStmtAttr(stmt, SQL_ATTR_PARAM_STATUS_PTR, nullptr, 0);
            ::SQLSetStmtAttr(stmt, SQL_ATTR_PARAMS_PROCESSED_PTR, nullptr, 0);

            const wstring query = wstring(L"SELECT COUNT_BIG(*) FROM ") + table;
            Exec(query.c_str());

            int64 count = -1;
            SQLLEN indicator = 0;
            ::SQLBindCol(stmt, 1, SQL_C_SBIGINT, &count, 0, &indicator);
            ::SQLFetch(stmt);
            ::SQLFreeStmt(stmt, SQL_CLOSE);
            return count;
        }

        void PrintDiagnostics()
        {
            SQLWCHAR state[6] = {};
            SQLINTEGER nativeError = 0;
            SQLWCHAR message[512] = {};
            SQLSMALLINT length = 0;
            for (SQLSMALLINT i = 1; ::SQLGetDiagRecW(SQL_HANDLE_STMT, stmt, i, state, &nativeError, message, 512, &length) == SQL_SUCCESS; i++)
                std::wcout << L"  진단 " << i << L": [" << state << L"] " << message << L'\n';
        }
    };

    void PrintResult(const char* caseName, SQLRETURN ret, SQLULEN processed, const SQLUSMALLINT (&statuses)[ROW_COUNT], int64 savedRows)
    {
        std::cout << caseName << '\n';
        std::cout << "  SQLExecDirect 반환값: " << ret << " (0=SUCCESS, 1=SUCCESS_WITH_INFO, -1=ERROR, 100=NO_DATA)\n";
        std::cout << "  처리된 파라미터 집합 수: " << processed << " / " << ROW_COUNT << '\n';
        for (SQLULEN i = 0; i < ROW_COUNT; i++)
            std::cout << "  행 " << i << " 상태: " << statuses[i] << " (0=SUCCESS, 5=ERROR, 6=SUCCESS_WITH_INFO, 7=UNUSED)\n";
        std::cout << "  대상 테이블에 남은 행 수: " << savedRows << '\n';
    }

    void BindStatusArrays(SQLHSTMT stmt, SQLULEN* processed, SQLUSMALLINT* statuses)
    {
        ::SQLSetStmtAttr(stmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)ROW_COUNT, 0);
        ::SQLSetStmtAttr(stmt, SQL_ATTR_PARAM_STATUS_PTR, statuses, 0);
        ::SQLSetStmtAttr(stmt, SQL_ATTR_PARAMS_PROCESSED_PTR, processed, 0);
    }
}

// 가장 단순한 경우: 세 행 중 두 번째 행이 기본 키 중복으로 실패한다.
TEST(DriverReproTest, DISABLED_TD040_PlainInsertWithOneFailingRow)
{
    ReproConnection conn;
    ASSERT_TRUE(conn.Open()) << "DB에 연결하지 못했다. 게임 서버와 같은 접속 문자열을 쓴다";

    conn.Exec(L"CREATE TABLE #Repro (id INT PRIMARY KEY)");

    int32 ids[ROW_COUNT] = { 1, 1, 2 };
    SQLLEN indicators[ROW_COUNT] = {};
    SQLULEN processed = 0;
    SQLUSMALLINT statuses[ROW_COUNT] = {};

    BindStatusArrays(conn.stmt, &processed, statuses);
    ::SQLBindParameter(conn.stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, ids, 0, indicators);

    const SQLRETURN ret = conn.Exec(L"INSERT INTO #Repro (id) VALUES (?)");
    conn.PrintDiagnostics();
    PrintResult("[1] INSERT 한 문장", ret, processed, statuses, conn.CountRows(L"#Repro"));
}

// 아이템 저장과 같은 모양: 임시 테이블에 넣고 MERGE한 뒤 지운다. 두 번째 행이 대상 테이블의 CHECK에 걸린다.
TEST(DriverReproTest, DISABLED_TD040_MergeBatchWithOneFailingRow)
{
    ReproConnection conn;
    ASSERT_TRUE(conn.Open()) << "DB에 연결하지 못했다. 게임 서버와 같은 접속 문자열을 쓴다";

    conn.Exec(L"CREATE TABLE #Target (character_id BIGINT, slot_id INT, template_id INT, count INT CHECK (count >= 0), PRIMARY KEY (character_id, slot_id))");

    int64 characterIds[ROW_COUNT] = { 1, 1, 1 };
    int32 slotIds[ROW_COUNT] = { 0, 1, 2 };
    int32 templateIds[ROW_COUNT] = { 2000, 2000, 2000 };
    int32 counts[ROW_COUNT] = { 3, -1, 5 };
    SQLLEN indicators[4][ROW_COUNT] = {};
    SQLULEN processed = 0;
    SQLUSMALLINT statuses[ROW_COUNT] = {};

    BindStatusArrays(conn.stmt, &processed, statuses);
    ::SQLBindParameter(conn.stmt, 1, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, characterIds, 0, indicators[0]);
    ::SQLBindParameter(conn.stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, slotIds, 0, indicators[1]);
    ::SQLBindParameter(conn.stmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, templateIds, 0, indicators[2]);
    ::SQLBindParameter(conn.stmt, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, counts, 0, indicators[3]);

    const SQLRETURN ret = conn.Exec(LR"SQL(
        SELECT * INTO #TempTable FROM #Target WHERE 1 = 0;
        INSERT INTO #TempTable (character_id, slot_id, template_id, count) VALUES (?, ?, ?, ?);
        MERGE INTO #Target AS T
        USING #TempTable AS S
        ON (T.character_id = S.character_id AND T.slot_id = S.slot_id)
        WHEN MATCHED AND S.template_id = 0 THEN DELETE
        WHEN MATCHED THEN UPDATE SET template_id = S.template_id, count = S.count
        WHEN NOT MATCHED BY TARGET AND S.template_id > 0 THEN
            INSERT (character_id, slot_id, template_id, count) VALUES (S.character_id, S.slot_id, S.template_id, S.count);
        DROP TABLE #TempTable;
    )SQL");
    conn.PrintDiagnostics();
    PrintResult("[2] 아이템 저장 모양의 배치", ret, processed, statuses, conn.CountRows(L"#Target"));
}
