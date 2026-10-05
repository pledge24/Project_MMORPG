#pragma once
#include "DBConnection.h"

/*-------------------
	DBConnectionPool
--------------------*/

/**
 * DB 연결을 미리 만들어 두고 빌려주는 풀. 전역 객체 GDBConnectionPool 하나만 있다.
 * 여러 스레드가 동시에 빌리고 돌려줘도 된다.
 */
class DBConnectionPool
{
public:
	DBConnectionPool();
	~DBConnectionPool();

	/** ODBC 환경을 만들고 연결을 connectionCount개 만든다. DB 작업을 시작하기 전에 한 번 부른다. */
	bool					Connect(int32 connectionCount, const WCHAR* connectionString);
	void					Clear();

	/** 남은 연결이 없으면 nullptr를 돌려준다. 빌린 연결은 Push로 돌려준다(GameServer의 DBConnectionGuard가 대신한다). */
	DBConnection*			Pop();
	void					Push(DBConnection* connection);

private:
	MAKE_LOCK;
	SQLHENV					_environment = SQL_NULL_HANDLE;
	vector<DBConnection*>	_connections;
};

