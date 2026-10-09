#pragma once
#include <variant>
#include <deque>
#include "ServerCore/DB/DBConnection.h"

/** 가짜 연결이 주고받는 값. 정수형은 모두 int64, 실수형은 double로 다룬다. */
using FakeDBValue = variant<int64, double, wstring>;
using FakeDBRow = vector<FakeDBValue>;

/**
 * ODBC 없이 DAO를 부르는 가짜 연결. 테스트만 쓴다.
 * 바인딩한 변수의 주소를 기록해 두었다가, Execute 때 파라미터 값을 읽고 Fetch 때 준비한 행을 컬럼 변수에 쓴다.
 * 결과를 준비하지 않은 Execute는 행 없이 성공하고 GetRowCount는 1이다.
 */
class FakeDBConnection : public DBConnection
{
public:
    /** 다음 Execute의 결과. rows는 Fetch가 차례로 돌려준다. */
    void QueueResult(vector<FakeDBRow> rows, int32 rowCount = 1)
    {
        _results.push_back(Result{ false, std::move(rows), rowCount });
    }

    /** 다음 Execute를 실패시킨다. */
    void QueueExecuteFailure()
    {
        _results.push_back(Result{ true, {}, -1 });
    }

    /** 다음 Execute가 DBError가 아닌 표준 예외를 던진다. 드라이버나 바인딩 코드가 던지는 경우를 흉내 낸다. */
    void QueueExecuteException()
    {
        _results.push_back(Result{ false, {}, -1, true });
    }

    //~ 기록
    vector<wstring> executedQueries;
    /** Execute 때 읽은 파라미터. 실행마다 한 줄이고, 파라미터 번호는 0부터 센다. */
    vector<map<int32, FakeDBValue>> executedParams;
    int32 beginCount = 0;
    int32 commitCount = 0;
    int32 rollbackCount = 0;

    //~ Begin DBConnection Interface
    virtual bool Execute(const WCHAR* query) override
    {
        executedQueries.push_back(query);

        map<int32, FakeDBValue> params;
        for (const auto& [index, binding] : _params)
            params[index] = Read(binding);
        executedParams.push_back(std::move(params));

        _current = Result{};
        if (_results.empty() == false)
        {
            _current = std::move(_results.front());
            _results.pop_front();
        }

        if (_current.raise)
            throw runtime_error("FakeDBConnection이 던진 예외");

        return _current.fail == false;
    }

    virtual bool Fetch() override
    {
        if (_current.rows.empty())
            return false;

        const FakeDBRow row = std::move(_current.rows.front());
        _current.rows.erase(_current.rows.begin());

        for (const auto& [index, binding] : _columns)
        {
            if (index < static_cast<int32>(row.size()))
                Write(binding, row[index]);
        }

        return true;
    }

    virtual int32 GetRowCount() override { return _current.rowCount; }
    virtual void Unbind() override { _params.clear(); _columns.clear(); }
    virtual void SetParamSetSize(int32& rows) override {}

    virtual bool BeginTransaction() override { beginCount++; return true; }
    virtual bool Commit() override { commitCount++; return true; }
    virtual bool Rollback() override { rollbackCount++; return true; }
    //~ End DBConnection Interface

protected:
    virtual bool BindParam(SQLUSMALLINT paramIndex, SQLSMALLINT cType, SQLSMALLINT sqlType, SQLULEN len, SQLPOINTER ptr, SQLLEN* index) override
    {
        _params[paramIndex - 1] = Binding{ cType, len, ptr };
        return true;
    }

    virtual bool BindCol(SQLUSMALLINT columnIndex, SQLSMALLINT cType, SQLULEN len, SQLPOINTER value, SQLLEN* index) override
    {
        _columns[columnIndex - 1] = Binding{ cType, len, value };
        return true;
    }

private:
    struct Binding
    {
        SQLSMALLINT cType = 0;
        SQLULEN len = 0;
        SQLPOINTER ptr = nullptr;
    };

    struct Result
    {
        bool fail = false;
        vector<FakeDBRow> rows;
        int32 rowCount = 1;
        bool raise = false;
    };

    static FakeDBValue Read(const Binding& binding)
    {
        switch (binding.cType)
        {
        case SQL_C_SBIGINT: return *static_cast<int64*>(binding.ptr);
        case SQL_C_LONG: return static_cast<int64>(*static_cast<int32*>(binding.ptr));
        case SQL_C_SHORT: return static_cast<int64>(*static_cast<int16*>(binding.ptr));
        case SQL_C_TINYINT: return static_cast<int64>(*static_cast<int8*>(binding.ptr));
        case SQL_C_FLOAT: return static_cast<double>(*static_cast<float*>(binding.ptr));
        case SQL_C_DOUBLE: return *static_cast<double*>(binding.ptr);
        case SQL_C_WCHAR: return wstring(static_cast<const WCHAR*>(binding.ptr));
        default: return int64(0);
        }
    }

    static void Write(const Binding& binding, const FakeDBValue& value)
    {
        switch (binding.cType)
        {
        case SQL_C_SBIGINT: *static_cast<int64*>(binding.ptr) = get<int64>(value); break;
        case SQL_C_LONG: *static_cast<int32*>(binding.ptr) = static_cast<int32>(get<int64>(value)); break;
        case SQL_C_SHORT: *static_cast<int16*>(binding.ptr) = static_cast<int16>(get<int64>(value)); break;
        case SQL_C_TINYINT: *static_cast<int8*>(binding.ptr) = static_cast<int8>(get<int64>(value)); break;
        case SQL_C_FLOAT: *static_cast<float*>(binding.ptr) = static_cast<float>(get<double>(value)); break;
        case SQL_C_DOUBLE: *static_cast<double*>(binding.ptr) = get<double>(value); break;
        case SQL_C_WCHAR:
        {
            const wstring& text = get<wstring>(value);
            WCHAR* buffer = static_cast<WCHAR*>(binding.ptr);
            const size_t length = (std::min)(text.size(), static_cast<size_t>(binding.len));
            ::wmemcpy(buffer, text.c_str(), length);
            buffer[length] = L'\0';
            break;
        }
        default: break;
        }
    }

    map<int32, Binding> _params;
    map<int32, Binding> _columns;
    deque<Result> _results;
    Result _current;
};
