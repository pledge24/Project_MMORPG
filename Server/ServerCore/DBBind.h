#pragma once
#include "DBConnection.h"

/*----------------
      DBBind
-----------------*/

template<int32 C>
struct FullBits { enum { value = (1 << (C - 1)) | FullBits<C-1>::value }; };

template<>
struct FullBits<1> { enum { value = 1 }; };

template<>
struct FullBits<0> { enum { value = 0 }; };

template<int32 ParamCount, int32 ColumnCount>
class DBBind
{
public:
	DBBind(DBConnection& dbConnection, const WCHAR* query)
		: _dbConnection(dbConnection), _query(query)
	{
		::memset(_paramIndex, 0, sizeof(_paramIndex));
		::memset(_columnIndex, 0, sizeof(_columnIndex));
		_paramFlag = 0;
		_columnFlag = 0;
		dbConnection.Unbind();
	}

	bool Validate()
	{
		return _paramFlag == FullBits<ParamCount>::value && _columnFlag == FullBits<ColumnCount>::value;
	}

	bool Execute()
	{
		ASSERT_CRASH(Validate());
		return _dbConnection.Execute(_query);
	}

	bool Fetch()
	{
		return _dbConnection.Fetch();
	}

public:
    /* 기본: 일반 타입으로 넘겨주는 경우 */
	template<typename T>
	void BindParam(int32 idx, T& value)
	{
		_dbConnection.BindParam(idx + 1, &value, &_paramIndex[idx]);
		_paramFlag |= (1LL << idx);
	}

    /* 특수화: 문자열을 넘겨주는 경우 */
	void BindParam(int32 idx, const WCHAR* value)
	{
		_dbConnection.BindParam(idx + 1, value, &_paramIndex[idx]);
		_paramFlag |= (1LL << idx);
	}

 //   /* 특수화: 배열을 넘겨주는 경우(BYTE 버전) */
	//template<typename T, int32 N>
	//void BindParam(int32 idx, T(&value)[N])
	//{
	//	_dbConnection.BindParam(idx + 1, (const BYTE*)value, size32(T) * N, &_paramIndex[idx]);
	//	_paramFlag |= (1LL << idx);
	//}

    /* 특수화: 배열을 넘겨주는 경우 */
    template<typename T, int32 N>
    void BindParam(int32 idx, T(&value)[N])
    {
        SQLLEN indicators[N];
        for (int32 i = 0; i < N; i++)
            indicators[i] = 0; // not null.

        _dbConnection.BindParam(idx + 1, value, indicators);
        _paramFlag |= (1LL << idx);
    }

 //   /* 특수화: T타입이 N개 들어있는 배열의 시작 주소를 넘겨주는 경우(BYTE 버전) */
	//template<typename T>
	//void BindParam(int32 idx, T* value, int32 N)
	//{
	//	_dbConnection.BindParam(idx + 1, (const BYTE*)value, size32(T) * N, &_paramIndex[idx]);
	//	_paramFlag |= (1LL << idx);
	//}

    /* 특수화: T타입이 N개 들어있는 배열의 시작 주소를 넘겨주는 경우 */
    template<typename T>
    void BindParam(int32 idx, T* value, int32 N, SQLLEN* indicators)
    {
        _dbConnection.BindParam(idx + 1, value, indicators);
        _paramFlag |= (1LL << idx);
    }

    /* 기본: 일반 타입으로 넘겨주는 경우 */
	template<typename T>
	void BindCol(int32 idx, T& value)
	{
		_dbConnection.BindCol(idx + 1, &value, &_columnIndex[idx]);
		_columnFlag |= (1LL << idx);
	}

    /* 특수화: N 길이의 문자열 배열을 받아오는 경우 */
	template<int32 N>
	void BindCol(int32 idx, WCHAR(&value)[N])
	{
		_dbConnection.BindCol(idx + 1, value, N - 1, &_columnIndex[idx]);
		_columnFlag |= (1LL << idx);
	}

    /* 특수화: len 길이의 문자열 포인터를 받아오는 경우 */
	void BindCol(int32 idx, WCHAR* value, int32 len)
	{
		_dbConnection.BindCol(idx + 1, value, len - 1, &_columnIndex[idx]);
		_columnFlag |= (1LL << idx);
	}

    /* 특수화: N 길이의 T타입 배열을 받아오는 경우 */
	template<typename T, int32 N>
	void BindCol(int32 idx, T(&value)[N])
	{
		_dbConnection.BindCol(idx + 1, value, size32(T) * N, &_columnIndex[idx]);
		_columnFlag |= (1LL << idx);
	}

protected:
	DBConnection&	_dbConnection;
	const WCHAR*	_query;
	SQLLEN			_paramIndex[ParamCount > 0 ? ParamCount : 1];
	SQLLEN			_columnIndex[ColumnCount > 0 ? ColumnCount : 1];
	uint64			_paramFlag;
	uint64			_columnFlag;
};

