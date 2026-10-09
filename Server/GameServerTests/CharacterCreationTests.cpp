#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Characters/CharacterCreation.h"
#include "Utils/EncodingConverter.h"

/*--------------------------------------------------------------
    캐릭터 생성 검증 테스트

    레벨 표가 없는 직업으로 캐릭터를 만들면 생성 쿼리가 레벨 1 능력치를 읽다가 서버가
    죽는다. 클라이언트의 마법사 버튼이 그 경로다. 이름이 DB 열 한도를 넘으면 생성이
    내부 오류로 끝난다.

    픽스처 결합도: Gamedata::s_classLevelDataTableMappings를 손으로 시드한다.
    전사 표에는 레벨 1 행 하나만 넣고, NONE은 실제 서버처럼 빈 표를 가리키게 한다.
---------------------------------------------------------------*/

namespace
{
    Protocol::CharacterOverview MakeCharacter(int32 classId, const string& name)
    {
        Protocol::CharacterOverview character;
        character.set_class_(static_cast<Protocol::CharacterClass>(classId));
        character.set_name(name);
        return character;
    }

    // UTF-16 기준 count자인 한글 이름을 UTF-8로 만든다. 한 글자가 UTF-8로 3바이트다.
    string MakeHangulName(int32 count)
    {
        wstring name(count, L'가');
        return EncodingConverter::WCharToString(name.c_str());
    }
}

class CharacterCreationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        warriorTable[1] = Json::object();
        Gamedata::s_classLevelDataTableMappings = {
            { Protocol::CLASS_TYPE_NONE, &emptyTable },
            { Protocol::CLASS_TYPE_WARRIOR, &warriorTable },
        };
    }

    void TearDown() override
    {
        Gamedata::s_classLevelDataTableMappings.clear();
    }

    DataTable emptyTable;
    DataTable warriorTable;
};

TEST_F(CharacterCreationTest, WarriorWithValidNamePasses)
{
    EXPECT_FALSE(CharacterCreation::Validate(MakeCharacter(Protocol::CLASS_TYPE_WARRIOR, "전사")).has_value());
}

TEST_F(CharacterCreationTest, ClassWithoutLevelTableIsRejected)
{
    const auto cause = CharacterCreation::Validate(MakeCharacter(Protocol::CLASS_TYPE_MAGE, "마법사"));

    ASSERT_TRUE(cause.has_value()) << "레벨 표가 없는 직업을 통과시키면 생성 쿼리가 널 표를 역참조한다";
    EXPECT_EQ(cause.value(), "선택할 수 없는 직업입니다.");
    EXPECT_FALSE(Gamedata::s_classLevelDataTableMappings.contains(Protocol::CLASS_TYPE_MAGE))
        << "검증이 전역 표에 없는 직업을 끼워 넣으면 안 된다";
}

TEST_F(CharacterCreationTest, NoneClassWithEmptyTableIsRejected)
{
    EXPECT_TRUE(CharacterCreation::Validate(MakeCharacter(Protocol::CLASS_TYPE_NONE, "무직")).has_value())
        << "NONE은 매핑에 있지만 표가 비어 있다";
}

TEST_F(CharacterCreationTest, UndefinedClassNumberIsRejected)
{
    EXPECT_TRUE(CharacterCreation::Validate(MakeCharacter(99, "정의안됨")).has_value());
}

TEST_F(CharacterCreationTest, EmptyNameIsRejected)
{
    const auto cause = CharacterCreation::Validate(MakeCharacter(Protocol::CLASS_TYPE_WARRIOR, ""));

    ASSERT_TRUE(cause.has_value());
    EXPECT_EQ(cause.value(), "캐릭터 이름은 50이하여야 합니다.");
}

TEST_F(CharacterCreationTest, FiftyHangulCharactersPass)
{
    EXPECT_FALSE(CharacterCreation::Validate(MakeCharacter(Protocol::CLASS_TYPE_WARRIOR, MakeHangulName(50))).has_value())
        << "UTF-8 바이트(150)가 아니라 UTF-16 글자 수(50)로 세야 한다";
}

TEST_F(CharacterCreationTest, FiftyOneCharactersAreRejected)
{
    EXPECT_TRUE(CharacterCreation::Validate(MakeCharacter(Protocol::CLASS_TYPE_WARRIOR, MakeHangulName(51))).has_value())
        << "DB 열 한도를 넘으면 생성 쿼리가 내부 오류로 끝난다";
}
