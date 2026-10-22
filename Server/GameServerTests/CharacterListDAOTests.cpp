#include "Core/pch.h"
#include <gtest/gtest.h>
#include "FakeDBConnection.h"
#include "Lobby/CharacterListDAO.h"
#include "DB/DAOCommon.h"

/*--------------------------------------------------------------
    캐릭터 목록 DAO 테스트

    캐릭터 생성 쿼리는 빈 슬롯이 없거나 이름이 겹치면 거절한다. 거절은 클라이언트에 보낼 결과이고,
    DB 작업의 실패는 예외(DBError)다. 둘이 한 오류 코드에 섞여 있으면 내부 오류 문구가 화면에 나가거나
    거절이 서버 오류로 기록된다.

    생성 쿼리는 캐릭터, 상태, 골드 행의 값을 기획 데이터에서 받는다. 바인딩 번호가 어긋나면
    시작 좌표가 HP 자리에 들어가는 식으로 값이 다른 열에 저장되고, 쿼리는 그대로 성공한다.

    픽스처 결합도: FakeDBConnection과 Gamedata::Install로 넣은 전사 레벨 표, 캐릭터 생성 표, 골드 재화 번호를 쓴다.
    SQL 문장과 이름 중복(고유 인덱스 위반) 판정은 실제 DB 테스트가 본다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 ACCOUNT_ID = 7;
    constexpr int64 CHARACTER_ID = 5'000'000'000;

    constexpr int32 START_MAP_ID = 1111;
    constexpr float START_X = 10.5f;
    constexpr float START_Y = -20.25f;
    constexpr float START_Z = 30.f;
    constexpr float START_YAW = 90.f;
    constexpr int64 START_GOLD = 6'000'000'000;
    constexpr int32 GOLD_CURRENCY_ID = 3;
    constexpr int32 LEVEL_ONE_HP = 120;
    constexpr int32 LEVEL_ONE_MP = 45;

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
        level1.maxHp = LEVEL_ONE_HP;
        level1.maxMp = LEVEL_ONE_MP;

        GamedataTables tables;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1 });
        tables.goldCurrencyId = GOLD_CURRENCY_ID;
        tables.characterCreation.startMapId = START_MAP_ID;
        tables.characterCreation.startPos = { START_X, START_Y, START_Z };
        tables.characterCreation.startYaw = START_YAW;
        tables.characterCreation.startGold = START_GOLD;
        tables.characterCreation.defaultCharacterSlotCount = 4;
        Gamedata::Install(std::move(tables));
    }

    void TearDown() override
    {
        Gamedata::Install(GamedataTables());
    }

    FakeDBConnection conn;
};

TEST_F(CharacterListDAOTest, CreatedCharacterReturnsIssuedId)
{
    conn.QueueResult({ { int64(0) } });

    const CreateCharacterResult result = CharacterListDAO::CreateCharacter(conn, ACCOUNT_ID, CHARACTER_ID, MakeWarrior());

    EXPECT_EQ(result.characterId, CHARACTER_ID) << "캐릭터 ID는 DB가 아니라 발급기가 준 값이다";
    EXPECT_FALSE(result.rejection.has_value());
}

TEST_F(CharacterListDAOTest, CreateBindsStartValuesFromGamedata)
{
    conn.QueueResult({ { int64(0) } });

    CharacterListDAO::CreateCharacter(conn, ACCOUNT_ID, CHARACTER_ID, MakeWarrior());

    ASSERT_EQ(conn.executedParams.size(), 1u);
    const map<int32, FakeDBValue>& params = conn.executedParams[0];
    EXPECT_EQ(get<int64>(params.at(0)), ACCOUNT_ID);
    EXPECT_EQ(get<int64>(params.at(1)), CHARACTER_ID) << "int32로 바인딩하면 Snowflake ID가 잘린다";
    EXPECT_EQ(get<int64>(params.at(2)), Protocol::CLASS_TYPE_WARRIOR);
    EXPECT_EQ(get<wstring>(params.at(3)), L"전사");
    EXPECT_EQ(get<int64>(params.at(4)), START_MAP_ID);
    EXPECT_FLOAT_EQ(static_cast<float>(get<double>(params.at(5))), START_X);
    EXPECT_FLOAT_EQ(static_cast<float>(get<double>(params.at(6))), START_Y);
    EXPECT_FLOAT_EQ(static_cast<float>(get<double>(params.at(7))), START_Z);
    EXPECT_FLOAT_EQ(static_cast<float>(get<double>(params.at(8))), START_YAW);
    EXPECT_EQ(get<int64>(params.at(9)), LEVEL_ONE_HP) << "시작 HP는 레벨 표의 1레벨 최대 HP다";
    EXPECT_EQ(get<int64>(params.at(10)), LEVEL_ONE_MP) << "시작 MP는 레벨 표의 1레벨 최대 MP다";
    EXPECT_EQ(get<int64>(params.at(11)), GOLD_CURRENCY_ID);
    EXPECT_EQ(get<int64>(params.at(12)), START_GOLD) << "int32로 바인딩하면 시작 골드가 잘린다";
}

TEST_F(CharacterListDAOTest, RejectionIsResultNotError)
{
    for (CharacterRejection rejection : { CharacterRejection::DUPLICATE_NAME, CharacterRejection::NO_EMPTY_SLOT })
    {
        FakeDBConnection rejectingConn;
        rejectingConn.QueueResult({ { int64(rejection) } });

        const CreateCharacterResult result = CharacterListDAO::CreateCharacter(rejectingConn, ACCOUNT_ID, CHARACTER_ID, MakeWarrior());

        ASSERT_TRUE(result.rejection.has_value());
        EXPECT_EQ(result.rejection.value(), rejection);
        EXPECT_EQ(result.characterId, 0) << "거절된 생성은 캐릭터 ID를 돌려주지 않는다";
        EXPECT_FALSE(ToMessage(result.rejection.value()).empty()) << "거절 사유는 생성 화면에 그대로 보여 준다";
    }
}

TEST_F(CharacterListDAOTest, UnknownResultCodeIsDBError)
{
    conn.QueueResult({ { int64(12345) } });

    EXPECT_THROW(CharacterListDAO::CreateCharacter(conn, ACCOUNT_ID, CHARACTER_ID, MakeWarrior()), DBError)
        << "쿼리가 바인딩하지 않은 사유를 돌려주면 쿼리가 잘못된 것이다. 화면에 보낼 사유가 아니다";
}

TEST_F(CharacterListDAOTest, FailedCreateIsDBError)
{
    conn.QueueExecuteFailure();

    EXPECT_THROW(CharacterListDAO::CreateCharacter(conn, ACCOUNT_ID, CHARACTER_ID, MakeWarrior()), DBError)
        << "고유 인덱스 위반이 아닌 실패(계정 행 없음 등)는 쿼리가 다시 던지므로 실행 실패로 온다";
}

TEST_F(CharacterListDAOTest, ClassWithoutLevelTableIsDBError)
{
    Protocol::CharacterOverview mage = MakeWarrior();
    mage.set_class_(Protocol::CLASS_TYPE_MAGE);

    EXPECT_THROW(CharacterListDAO::CreateCharacter(conn, ACCOUNT_ID, CHARACTER_ID, mage), DBError);
    EXPECT_TRUE(conn.executedQueries.empty()) << "시작 HP를 모르는 캐릭터는 넣지 않는다";
}

TEST_F(CharacterListDAOTest, LoadCharacterListReadsRows)
{
    conn.QueueResult({
        { CHARACTER_ID, int64(Protocol::CLASS_TYPE_WARRIOR), wstring(L"전사"), int64(12) },
        { CHARACTER_ID + 1, int64(Protocol::CLASS_TYPE_WARRIOR), wstring(L"둘째"), int64(1) },
    });

    vector<Protocol::CharacterOverview> characters;
    CharacterListDAO::LoadCharacterList(conn, ACCOUNT_ID, OUT characters);

    ASSERT_EQ(characters.size(), 2u);
    EXPECT_EQ(characters[0].character_id(), CHARACTER_ID);
    EXPECT_EQ(characters[0].class_(), Protocol::CLASS_TYPE_WARRIOR);
    EXPECT_EQ(characters[0].name(), "전사");
    EXPECT_EQ(characters[0].level(), 12);
    EXPECT_EQ(characters[1].name(), "둘째");
    EXPECT_EQ(get<int64>(conn.executedParams[0].at(0)), ACCOUNT_ID);
}

TEST_F(CharacterListDAOTest, FailedQueryIsDBError)
{
    conn.QueueExecuteFailure();

    vector<Protocol::CharacterOverview> characters;
    EXPECT_THROW(CharacterListDAO::LoadCharacterList(conn, ACCOUNT_ID, OUT characters), DBError);
}

TEST_F(CharacterListDAOTest, DeleteWithoutAffectedRowIsFalse)
{
    conn.QueueResult({}, 0);

    EXPECT_FALSE(CharacterListDAO::DeleteCharacter(conn, ACCOUNT_ID, CHARACTER_ID))
        << "지운 행이 없으면 이 계정의 캐릭터가 아니거나 이미 지운 캐릭터다";

    const map<int32, FakeDBValue>& params = conn.executedParams[0];
    EXPECT_EQ(get<int64>(params.at(0)), CHARACTER_ID);
    EXPECT_EQ(get<int64>(params.at(1)), ACCOUNT_ID);
}

TEST_F(CharacterListDAOTest, DeleteWithAffectedRowIsTrue)
{
    conn.QueueResult({}, 1);

    EXPECT_TRUE(CharacterListDAO::DeleteCharacter(conn, ACCOUNT_ID, CHARACTER_ID));
}

TEST_F(CharacterListDAOTest, OwnershipBindsCharacterAndAccount)
{
    conn.QueueResult({ { int64(1) } });

    EXPECT_TRUE(CharacterListDAO::OwnsCharacter(conn, ACCOUNT_ID, CHARACTER_ID));

    const map<int32, FakeDBValue>& params = conn.executedParams[0];
    EXPECT_EQ(get<int64>(params.at(0)), CHARACTER_ID);
    EXPECT_EQ(get<int64>(params.at(1)), ACCOUNT_ID) << "캐릭터 번호는 클라이언트 값이라 계정 번호를 함께 대조해야 한다";
}

TEST_F(CharacterListDAOTest, OwnershipWithoutMatchingRowIsFalse)
{
    conn.QueueResult({ { int64(0) } });

    EXPECT_FALSE(CharacterListDAO::OwnsCharacter(conn, ACCOUNT_ID, CHARACTER_ID));
}

TEST_F(CharacterListDAOTest, FailedOwnershipQueryIsDBError)
{
    conn.QueueExecuteFailure();

    EXPECT_THROW(CharacterListDAO::OwnsCharacter(conn, ACCOUNT_ID, CHARACTER_ID), DBError);
}
