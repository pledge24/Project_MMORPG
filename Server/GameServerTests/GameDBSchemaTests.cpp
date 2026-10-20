#include "Core/pch.h"
#include <gtest/gtest.h>
#include "GameDBTestFixture.h"

/*--------------------------------------------------------------
    GameDB 스키마 확인 (실제 DB)

    GameDB_Test에 연결하고, 마이그레이션이 만든 테이블을 조회할 수 있는지 본다.
    행을 넣지 않으므로 지울 것도 없다.

    기본으로 꺼져 있다. 접속 문자열과 실행 필터는 GameDBTestFixture에 있다.
---------------------------------------------------------------*/

class DISABLED_GameDBSchemaTest : public GameDBTestFixture
{
};

TEST_F(DISABLED_GameDBSchemaTest, ReadsEveryTableOfInitialSchema)
{
    const vector<wstring> tables = {
        L"dbo.Account",
        L"dbo.[Character]",
        L"dbo.CharacterState",
        L"dbo.Item",
        L"dbo.ItemGear",
        L"dbo.ItemGearOption",
        L"dbo.CharacterCurrency",
    };

    for (const wstring& table : tables)
    {
        const wstring query = L"SELECT TOP (0) * FROM " + table;
        EXPECT_TRUE(Execute(query)) << EncodingConverter::WCharToString(table.c_str()) << "를 조회하지 못했다";
        EXPECT_FALSE(Connection().Fetch()) << "TOP (0)은 행을 돌려주지 않는다";
    }
}
