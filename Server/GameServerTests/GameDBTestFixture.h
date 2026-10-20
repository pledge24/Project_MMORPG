#pragma once
#include <gtest/gtest.h>
#include "ServerCore/DB/DBConnection.h"
#include "Core/Config.h"
#include "Utils/EncodingConverter.h"

/**
 * GameDB_Test에 붙는 실제 DB 테스트의 픽스처.
 *
 * P1_GAME_DB_TEST_CONNECTION_STRING의 접속 문자열로 연결 하나를 열어 테스트에 넘긴다. 게임 서버의
 * P1_GAME_DB_CONNECTION_STRING과 기본값은 읽지 않는다. 행을 지우는 테스트가 개발용 GameDB에 붙지 않게 하려고
 * 변수를 나눴고, 연결된 DB의 이름이 GameDB_Test가 아니어도 테스트를 시작하지 않는다.
 *
 * 테스트가 넣은 행은 AddCleanup으로 지울 문장을 넣어 두면 TearDown이 넣은 순서의 역순으로 지운다.
 * 테스트가 실패해도 지운다. 다른 테스트나 사람이 넣은 행은 건드리지 않는다.
 * 이 픽스처를 쓰는 묶음은 이름을 DISABLED_GameDB로 시작한다. 그래야 단위 테스트를 돌릴 때 빠지고, 아래 필터 하나로 모두 돈다.
 *
 *     GameServerTests.exe --gtest_also_run_disabled_tests --gtest_filter=DISABLED_GameDB*
 */
class GameDBTestFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const std::optional<string> connectionString = Config::ReadProcessEnv(CONNECTION_STRING_ENV);
        ASSERT_TRUE(connectionString.has_value()) << CONNECTION_STRING_ENV << " 환경 변수에 테스트 DB의 접속 문자열을 넣는다";

        ASSERT_EQ(::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_environment), SQL_SUCCESS);
        ASSERT_EQ(::SQLSetEnvAttr(_environment, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0), SQL_SUCCESS);

        const wstring wideConnectionString = EncodingConverter::StringToWString(connectionString.value());
        ASSERT_TRUE(_connection.Connect(_environment, wideConnectionString.c_str())) << "테스트 DB에 연결하지 못했다";

        const wstring database = QueryCurrentDatabase();
        ASSERT_EQ(database, EXPECTED_DATABASE) << "행을 지우는 테스트이므로 테스트 DB가 아닌 DB에서는 돌리지 않는다";
    }

    void TearDown() override
    {
        for (auto it = _cleanups.rbegin(); it != _cleanups.rend(); ++it)
        {
            EXPECT_TRUE(Execute(*it)) << "테스트가 넣은 행을 지우지 못했다: " << EncodingConverter::WCharToString(it->c_str());
        }

        _connection.Clear();
        if (_environment != SQL_NULL_HANDLE)
        {
            ::SQLFreeHandle(SQL_HANDLE_ENV, _environment);
            _environment = SQL_NULL_HANDLE;
        }
    }

    /** 테스트가 넣은 행을 지우는 문장을 넣는다. 행을 넣기 전에 부르면 넣다가 실패해도 지운다. */
    void AddCleanup(wstring query)
    {
        _cleanups.push_back(std::move(query));
    }

    /** 앞 문장의 바인딩과 커서를 풀고 query를 실행한다. 결과 열을 바인딩하려면 Connection()을 직접 쓴다. */
    bool Execute(const wstring& query)
    {
        _connection.Unbind();
        return _connection.Execute(query.c_str());
    }

    DBConnection& Connection() { return _connection; }

private:
    wstring QueryCurrentDatabase()
    {
        _connection.Unbind();
        WCHAR name[128] = {};
        SQLLEN nameLength = 0;
        _connection.BindCol(1, name, sizeof(name), &nameLength);
        if (_connection.Execute(L"SELECT DB_NAME()") == false || _connection.Fetch() == false)
            return L"";

        return name;
    }

private:
    static constexpr const char* CONNECTION_STRING_ENV = "P1_GAME_DB_TEST_CONNECTION_STRING";
    static constexpr const WCHAR* EXPECTED_DATABASE = L"GameDB_Test";

    SQLHENV         _environment = SQL_NULL_HANDLE;
    DBConnection    _connection;
    vector<wstring> _cleanups;
};
