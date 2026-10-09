#include "Core/pch.h"
#include <gtest/gtest.h>
#include "FakeDBConnection.h"
#include "DB/CharacterListDAO.h"
#include "DB/DAOCommon.h"

/*--------------------------------------------------------------
    캐릭터 목록 DAO 테스트

    캐릭터 생성 쿼리는 빈 슬롯이 없거나 이름이 겹치면 거절한다. 거절은 클라이언트에 보낼 결과이고,
    DB 작업의 실패는 예외(DBError)다. 둘이 한 오류 코드에 섞여 있으면 내부 오류 문구가 화면에 나가거나
    거절이 서버 오류로 기록된다.

    픽스처 결합도: FakeDBConnection과 1레벨짜리 전사 레벨 표(Gamedata::Install)만 쓴다.
    생성 쿼리는 첫 레벨의 스탯으로 마지막 상태 행을 만들므로 레벨 표가 있어야 한다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 USER_ID = 7;

    Protocol::CharacterOverview MakeWarrior()
    {
        Protocol::CharacterOverview character;
        character.set_class_(Protocol::CLASS_TYPE_WARRIOR);
        character.set_name("전사");
        return character;
    }
}

class CharacterListDAOTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        LevelTemplate level1;
        level1.level = 1;

        GamedataTables tables;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1 });
        Gamedata::Install(std::move(tables));
    }

    void TearDown() override
    {
        Gamedata::Install(GamedataTables());
    }

    FakeDBConnection conn;
};

TEST_F(CharacterListDAOTest, CreatedCharacterReturnsItsId)
{
    conn.QueueResult({ { int64(99) } });

    const CreateCharacterResult result = CharacterListDAO::CreateCharacter(conn, MakeWarrior(), USER_ID);

    EXPECT_EQ(result.characterId, 99);
    EXPECT_FALSE(result.rejection.has_value());
}

TEST_F(CharacterListDAOTest, RejectionIsResultNotError)
{
    // 쿼리는 거절 사유의 부호를 뒤집어 character_id 자리에 돌려준다.
    conn.QueueResult({ { -int64(CharacterRejection::NO_EMPTY_SLOT) } });

    const CreateCharacterResult result = CharacterListDAO::CreateCharacter(conn, MakeWarrior(), USER_ID);

    ASSERT_TRUE(result.rejection.has_value());
    EXPECT_EQ(result.rejection.value(), CharacterRejection::NO_EMPTY_SLOT);
    EXPECT_FALSE(ToMessage(result.rejection.value()).empty()) << "거절 사유는 생성 화면에 그대로 보여 준다";
}

TEST_F(CharacterListDAOTest, UnknownRejectionCodeIsDBError)
{
    conn.QueueResult({ { int64(-12345) } });

    EXPECT_THROW(CharacterListDAO::CreateCharacter(conn, MakeWarrior(), USER_ID), DBError)
        << "쿼리가 바인딩하지 않은 사유를 돌려주면 쿼리가 잘못된 것이다. 화면에 보낼 사유가 아니다";
}

TEST_F(CharacterListDAOTest, FailedQueryIsDBError)
{
    conn.QueueExecuteFailure();

    vector<Protocol::CharacterOverview> characters;
    EXPECT_THROW(CharacterListDAO::LoadCharacterList(conn, USER_ID, OUT characters), DBError);
}
