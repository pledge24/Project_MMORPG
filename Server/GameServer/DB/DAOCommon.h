#pragma once

/**
 * DAO가 DB 작업 중에 던지고 받는 오류 코드. 값마다 DBErrorCauseMappings에 사유 문구가 있다.
 * 새 값을 더하면 DBErrorCauseMappings에도 넣는다. 빠지면 PrintDBErrorLog의 at()이 예외를 던진다.
 */
enum DBCustomError
{
    NONE = 24000,
    SQL_EXECUTE_FAIL = 24001,
    SQL_FETCH_FAIL = 24002,
    ALREADY_EXISTING_CHARACTER = 24003,
    SQL_MISMATCHED_GET_ROW_COUNT = 24004,
    SQL_MISMATCHED_PROCESSED_PARAMSET_SIZE = 24005,
    INVENTORY_DIRTY_FLAGS_NOT_FOUND = 24006,
    UNKNOWN_CHARACTER_CLASS = 24007,
    NO_EMPTY_CHARACTER_SLOT = 24008
};

inline const unordered_map<DBCustomError, wstring> DBErrorCauseMappings =
{
    {NONE, L""},
    {SQL_EXECUTE_FAIL, L"Execute() false 반환"},
    {SQL_FETCH_FAIL, L"Fetch() false 반환"},
    {ALREADY_EXISTING_CHARACTER, L"이미 존재하는 캐릭터입니다."},
    {SQL_MISMATCHED_GET_ROW_COUNT, L"GetRowCount() 불일치 발생"},
    {SQL_MISMATCHED_PROCESSED_PARAMSET_SIZE, L"파라미터 배열 처리 행 수 불일치 발생"},
    {INVENTORY_DIRTY_FLAGS_NOT_FOUND, L"더티 플래그 표에 없는 아이템 타입"},
    {UNKNOWN_CHARACTER_CLASS, L"레벨 표에 없는 직업"},
    {NO_EMPTY_CHARACTER_SLOT, L"빈 캐릭터 슬롯이 없습니다."},
};

inline void PrintDBErrorLog(const DBCustomError error)
{
    wcout << L"오류 발생: " << error << L"(" << DBErrorCauseMappings.at(error) << L")" << endl;
}

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
