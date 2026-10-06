#pragma once
#include <sql.h>
#include <sqlext.h>

/*----------------
	DBConnection
-----------------*/

/** 이 길이를 넘는 문자열과 바이너리는 LONG 타입으로 바인딩한다. */
enum
{
	WVARCHAR_MAX = 4000,
	BINARY_MAX = 8000
};

/** ODBC 진단 레코드 하나. 실패한 호출의 SQLSTATE와 원본 오류 번호, 메시지를 담는다. */
struct DiagnosticInfo
{
    wstring sqlState;
    SQLINTEGER nativeError;
    wstring message;
};

/**
 * ODBC 연결 하나와 statement 하나를 묶은 클래스. DBConnectionPool이 만들어 들고 있다.
 * statement가 하나뿐이므로 한 번에 한 스레드만 쓴다. DAO는 풀에서 빌려 쓰고 돌려준다.
 * 보통은 직접 바인딩하지 않고 DBBind를 거쳐 쓴다.
 */
class DBConnection
{
public:
	/** henv 위에서 연결하고 statement를 할당한다. */
	bool			Connect(SQLHENV henv, const WCHAR* connectionString);
	void			Clear();

	/** 쿼리를 바로 실행한다. 영향받은 행이 없을 때(SQL_NO_DATA)도 성공으로 본다. 실행 전에 진단 기록을 비운다. */
	bool			Execute(const WCHAR* query);
	/** 다음 행을 바인딩한 변수로 읽는다. 행이 더 없거나 실패하면 false를 돌려준다. */
	bool			Fetch();
    /** 수정 쿼리(UPDATE, INSERT, DELETE)에 영향을 받은 행의 수 반환(SELECT는 -1 반환)*/
	int32			GetRowCount();
	/** 바인딩을 모두 풀고 커서를 닫는다. PARAMSET_SIZE와 ROW_ARRAY_SIZE도 1로 되돌린다. */
	void			Unbind();
    /** 배열 파라미터의 행 수를 정한다. DBBind 생성자가 Unbind로 1로 되돌리므로 DBBind를 만든 뒤에 부른다. */
    void            SetParamSetSize(int32& rows);

    /** 마지막 Execute 이후 쌓인 진단에 이 SQLSTATE나 원본 오류 번호가 있는지 본다. */
    bool            FindError(const SQLWCHAR* sqlState);
    bool            FindError(const wstring& targetState);
    bool            FindError(SQLINTEGER nativeError);

public:
	bool			BindParam(int32 paramIndex, bool* value, SQLLEN* index);
	bool			BindParam(int32 paramIndex, float* value, SQLLEN* index);
	bool			BindParam(int32 paramIndex, double* value, SQLLEN* index);
	bool			BindParam(int32 paramIndex, int8* value, SQLLEN* index);
	bool			BindParam(int32 paramIndex, int16* value, SQLLEN* index);
	bool			BindParam(int32 paramIndex, int32* value, SQLLEN* index);
	bool			BindParam(int32 paramIndex, int64* value, SQLLEN* index);
	bool			BindParam(int32 paramIndex, TIMESTAMP_STRUCT* value, SQLLEN* index);
	bool			BindParam(int32 paramIndex, const WCHAR* str, SQLLEN* index);
	bool			BindParam(int32 paramIndex, const BYTE* bin, int32 size, SQLLEN* index);

	bool			BindCol(int32 columnIndex, bool* value, SQLLEN* index);
	bool			BindCol(int32 columnIndex, float* value, SQLLEN* index);
	bool			BindCol(int32 columnIndex, double* value, SQLLEN* index);
	bool			BindCol(int32 columnIndex, int8* value, SQLLEN* index);
	bool			BindCol(int32 columnIndex, int16* value, SQLLEN* index);
	bool			BindCol(int32 columnIndex, int32* value, SQLLEN* index);
	bool			BindCol(int32 columnIndex, int64* value, SQLLEN* index);
	bool			BindCol(int32 columnIndex, TIMESTAMP_STRUCT* value, SQLLEN* index);
	bool			BindCol(int32 columnIndex, WCHAR* str, int32 size, SQLLEN* index);
	bool			BindCol(int32 columnIndex, BYTE* bin, int32 size, SQLLEN* index);

private:
	bool			BindParam(SQLUSMALLINT paramIndex, SQLSMALLINT cType, SQLSMALLINT sqlType, SQLULEN len, SQLPOINTER ptr, SQLLEN* index);
	bool			BindCol(SQLUSMALLINT columnIndex, SQLSMALLINT cType, SQLULEN len, SQLPOINTER value, SQLLEN* index);
	void			HandleError(SQLRETURN ret);

private:
	SQLHDBC			        _connection = SQL_NULL_HANDLE;
	SQLHSTMT		        _statement = SQL_NULL_HANDLE;
    vector<DiagnosticInfo>  _diagnostics;
};

