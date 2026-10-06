#pragma once
#include "DBConnection.h"

/*----------------
      DBBind
-----------------*/

/** 하위 C개 비트가 모두 켜진 값. DBBind가 바인딩 누락을 검사할 때 쓴다. */
template<int32 C>
struct FullBits { enum { value = (1 << (C - 1)) | FullBits<C-1>::value }; };

template<>
struct FullBits<1> { enum { value = 1 }; };

template<>
struct FullBits<0> { enum { value = 0 }; };

/**
 * 파라미터와 컬럼 개수를 템플릿 인자로 받아 쿼리 하나를 바인딩하고 실행한다.
 * 바인딩한 인덱스를 비트로 기록해서, 하나라도 빠지면 Execute에서 크래시한다.
 * 바인딩한 변수는 Execute와 Fetch가 끝날 때까지 살아 있어야 한다. 인덱스는 0부터 센다.
 */
template<int32 ParamCount, int32 ColumnCount>
class DBBind
{
public:
	/** 연결의 이전 바인딩을 모두 푼다(Unbind). 배열 파라미터의 SetParamSetSize는 이 뒤에 부른다. */
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

	/** 바인딩이 모두 되지 않았으면 크래시한다. */
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
	/** 기본: 일반 타입으로 넘겨주는 경우 */
	template<typename T>
	void BindParam(int32 idx, T& value)
	{
		_dbConnection.BindParam(idx + 1, &value, &_paramIndex[idx][0]);
		_paramFlag |= (1LL << idx);
	}

	/** 특수화: 문자열을 넘겨주는 경우 */
	void BindParam(int32 idx, const WCHAR* value)
	{
		_dbConnection.BindParam(idx + 1, value, &_paramIndex[idx][0]);
		_paramFlag |= (1LL << idx);
	}

	/** 특수화: 배열을 넘겨주는 경우 */
	template<typename T, int32 N>
	void BindParam(int32 idx, T(&value)[N])
	{
		_dbConnection.BindParam(idx + 1, (const BYTE*)value, size32(T) * N, &_paramIndex[idx][0]);
		_paramFlag |= (1LL << idx);
	}

	/** 특수화: T타입이 N개 들어있는 배열의 시작 주소를 넘겨주는 경우(BYTE 버전) */
	template<typename T>
	void BindParam(int32 idx, T* value, int32 N)
	{
		_dbConnection.BindParam(idx + 1, (const BYTE*)value, size32(T) * N, &_paramIndex[idx][0]);
		_paramFlag |= (1LL << idx);
	}

    /** 파라미터 배열: 배열 + 실제 데이터 개수를 넘겨주는 경우 */
    /** rows 뒤의 행은 NULL로 표시한다. 최대 MAX_ROWS(100)행이다. */
    template<typename T, int32 N>
    void BindParamSet(int32 idx, T(&value)[N], int32 rows)
    {
        for (int32 i = 0; i < MAX_ROWS; i++)
            _paramIndex[idx][i] = i < rows ? 0 : SQL_NULL_DATA;

        _dbConnection.BindParam(idx + 1, value, _paramIndex[idx]);
        _paramFlag |= (1LL << idx);
    }

	/** 기본: 일반 타입으로 넘겨주는 경우 */
	template<typename T>
	void BindCol(int32 idx, T& value)
	{
		_dbConnection.BindCol(idx + 1, &value, &_columnIndex[idx][0]);
		_columnFlag |= (1LL << idx);
	}

	/** 특수화: N 길이의 문자열 배열을 받아오는 경우 */
	template<int32 N>
	void BindCol(int32 idx, WCHAR(&value)[N])
	{
		_dbConnection.BindCol(idx + 1, value, N - 1, &_columnIndex[idx][0]);
		_columnFlag |= (1LL << idx);
	}

	/** 특수화: len 길이의 문자열 포인터를 받아오는 경우 */
	void BindCol(int32 idx, WCHAR* value, int32 len)
	{
		_dbConnection.BindCol(idx + 1, value, len - 1, &_columnIndex[idx][0]);
		_columnFlag |= (1LL << idx);
	}

	/** 특수화: N 길이의 T타입 배열을 받아오는 경우 */
	template<typename T, int32 N>
	void BindCol(int32 idx, T(&value)[N])
	{
		_dbConnection.BindCol(idx + 1, value, size32(T) * N, &_columnIndex[idx][0]);
		_columnFlag |= (1LL << idx);
	}

protected:
    static constexpr int32  MAX_ROWS = 100;

	DBConnection&	        _dbConnection;
	const WCHAR*	        _query;
	SQLLEN			        _paramIndex[ParamCount > 0 ? ParamCount : 1][MAX_ROWS];
	SQLLEN			        _columnIndex[ColumnCount > 0 ? ColumnCount : 1][MAX_ROWS];
	uint64			        _paramFlag;
	uint64			        _columnFlag;
};

