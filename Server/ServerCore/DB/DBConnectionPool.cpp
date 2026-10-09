#include "ServerCore/Core/pch.h"
#include "ServerCore/DB/DBConnectionPool.h"

/*-------------------
	DBConnectionPool
--------------------*/

DBConnectionPool::DBConnectionPool()
{

}

DBConnectionPool::~DBConnectionPool()
{
	Clear();
}

bool DBConnectionPool::Connect(int32 connectionCount, const WCHAR* connectionString)
{
	USE_LOCK

	if (::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_environment) != SQL_SUCCESS)
		return false;

	if (::SQLSetEnvAttr(_environment, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0) != SQL_SUCCESS)
		return false;

	for (int32 i = 0; i < connectionCount; i++)
	{
		DBConnection* connection = new DBConnection();
		if (connection->Connect(_environment, connectionString) == false)
		{
			delete connection;
			return false;
		}

		_connections.push_back(connection);
	}

	return true;
}

void DBConnectionPool::Clear()
{
    USE_LOCK

	// 연결 핸들은 환경 핸들에 딸려 있으므로 연결을 먼저 지운다. DBConnection의 소멸자가 연결을 끊고 핸들을 해제한다.
	for (DBConnection* connection : _connections)
		delete(connection);

	_connections.clear();

	if (_environment != SQL_NULL_HANDLE)
	{
		::SQLFreeHandle(SQL_HANDLE_ENV, _environment);
		_environment = SQL_NULL_HANDLE;
	}
}

DBConnection* DBConnectionPool::Pop()
{
    USE_LOCK

	if (_connections.empty())
		return nullptr;

	DBConnection* connection = _connections.back();
	_connections.pop_back();
	return connection;
}

void DBConnectionPool::Push(DBConnection* connection)
{
    USE_LOCK
	_connections.push_back(connection);
}
