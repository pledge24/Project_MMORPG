#pragma once

/**
 * DB 작업이 실패했다. 서버 쪽 문제이므로 클라이언트에는 사유를 보내지 않고 로그로만 남긴다.
 * DAO가 던지고, 진행 저장소와 DB 잡이 받는다. 클라이언트에 보낼 거절은 이 예외가 아니라 결과로 돌려준다.
 */
class DBError : public runtime_error
{
public:
    /** where에는 던진 곳의 이름을 넣는다. 보통 __func__이고, 생성자 안처럼 __func__이 다른 이름을 내면 문자열로 적는다. */
    DBError(string_view where, string_view cause) : runtime_error(format("{}: {}", where, cause)) {}
};

/**
 * 연결 풀에서 연결을 빌리고, 가드가 사라질 때 돌려준다.
 * 예외로 함수를 빠져나가도 연결이 풀로 돌아간다. DB 잡이 지역 변수로 만들어 DAO와 ProgressStorage에 연결을 넘긴다.
 * 풀이 비어 있으면 DBError를 던진다. 풀은 DB 스레드마다 하나씩 연결을 두므로, 잡 하나가 가드를 둘 이상 만들지 않는 한 비지 않는다.
 */
class DBConnectionGuard
{
public:
    DBConnectionGuard() : _connection(GDBConnectionPool->Pop())
    {
        if (_connection == nullptr)
            throw DBError("DBConnectionGuard", "연결 풀이 비어 있다");
    }
    ~DBConnectionGuard() { GDBConnectionPool->Push(_connection); }

    DBConnectionGuard(const DBConnectionGuard&) = delete;
    DBConnectionGuard& operator=(const DBConnectionGuard&) = delete;

    DBConnection& operator*() const { return *_connection; }
    DBConnection* operator->() const { return _connection; }

private:
    DBConnection* _connection;
};

/** 배열 파라미터 한 번에 묶을 수 있는 행 수. */
constexpr int32 MAX_PARAM_ROWS = DBBind<1, 0>::MAX_PARAM_ROWS;

/**
 * rows를 배열 파라미터로 묶어 query를 한 번 실행한다. 행이 없으면 실행하지 않는다.
 * Binding은 PARAMS(파라미터 수)를 갖고, (DBBind<PARAMS, 0>&, const vector<Row>&)로 만들면 행을 자기 배열에 옮겨 바인딩한다.
 * 행이 MAX_PARAM_ROWS를 넘거나, 실행이 실패하거나, 한 행이라도 실패하면 DBError. where는 오류 메시지에 붙일 호출자 이름이다.
 * 일부 행이 이미 반영됐을 수 있으므로 호출자는 트랜잭션 안에서 부르고, DBError를 받으면 되돌린다.
 * DBBind는 query를 가리키기만 하므로 query는 이 함수가 끝날 때까지 살아 있어야 한다.
 */
template<typename Binding, typename Row>
void ExecuteParamSet(DBConnection& conn, string_view where, const WCHAR* query, const vector<Row>& rows)
{
    if (rows.empty())
        return;

    // 넘는 행은 바인딩 배열 밖에 쓴다.
    if (rows.size() > static_cast<size_t>(MAX_PARAM_ROWS))
        throw DBError(where, format("저장할 행 {}개가 한 번에 묶을 수 있는 {}개를 넘는다", rows.size(), MAX_PARAM_ROWS));

    DBBind<Binding::PARAMS, 0> dbBind(conn, query);
    // 바인딩한 배열은 실행이 끝날 때까지 살아 있어야 한다. 스택에 두기에는 커서 힙에 둔다.
    unique_ptr<Binding> binding = make_unique<Binding>(dbBind, rows);

    // DBBind 생성자가 Unbind로 행 수를 1로 되돌리므로 바인딩한 뒤에 정한다.
    int32 rowCount = static_cast<int32>(rows.size());
    conn.SetParamSetSize(rowCount);

    // 일부 행만 실패해도 실행은 SQL_SUCCESS_WITH_INFO로 성공하고 나머지 행은 저장된다(ODBC Driver 17에서 확인).
    // 행별 결과를 받아 모든 행이 성공했는지 직접 본다.
    vector<SQLUSMALLINT> statuses(rows.size(), SQL_PARAM_UNUSED);
    SQLULEN processedCount = 0;
    conn.SetParamStatusArray(statuses.data(), &processedCount);

    if (dbBind.Execute() == false)
        throw DBError(where, "쿼리 실행 실패");

    if (processedCount != rows.size())
        throw DBError(where, format("{}행 중 {}행만 처리했다", rows.size(), processedCount));

    for (size_t i = 0; i < statuses.size(); i++)
    {
        if (statuses[i] != SQL_PARAM_SUCCESS && statuses[i] != SQL_PARAM_SUCCESS_WITH_INFO)
            throw DBError(where, format("{}행 중 {}번째 행이 실패했다(상태 {})", rows.size(), i + 1, statuses[i]));
    }
}
