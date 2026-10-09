#pragma once

/**
 * DB 작업이 실패했다. 서버 쪽 문제이므로 클라이언트에는 사유를 보내지 않고 로그로만 남긴다.
 * DAO가 던지고, 진행 저장소와 DB 잡이 받는다. 클라이언트에 보낼 거절은 이 예외가 아니라 결과로 돌려준다.
 */
class DBError : public runtime_error
{
public:
    /** where에는 던진 함수 이름(__func__)을 넣는다. */
    DBError(string_view where, string_view cause) : runtime_error(format("{}: {}", where, cause)) {}
};

/**
 * 연결 풀에서 연결을 빌리고, 가드가 사라질 때 돌려준다.
 * 예외로 함수를 빠져나가도 연결이 풀로 돌아간다. DB 잡이 지역 변수로 만들어 DAO와 ProgressStorage에 연결을 넘긴다.
 */
class DBConnectionGuard
{
public:
    DBConnectionGuard() : _connection(GDBConnectionPool->Pop()) {}
    ~DBConnectionGuard() { GDBConnectionPool->Push(_connection); }

    DBConnectionGuard(const DBConnectionGuard&) = delete;
    DBConnectionGuard& operator=(const DBConnectionGuard&) = delete;

    DBConnection& operator*() const { return *_connection; }
    DBConnection* operator->() const { return _connection; }

private:
    DBConnection* _connection;
};
