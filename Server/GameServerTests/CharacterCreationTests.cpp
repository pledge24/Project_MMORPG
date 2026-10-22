#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Lobby/CharacterCreation.h"
#include "Utils/EncodingConverter.h"
#include "Lobby/Data/LobbyData.h"

/*--------------------------------------------------------------
    캐릭터 생성 검증 테스트

    로비 표의 생성할 수 있는 직업이 아니면 거절한다. 그런 직업의 캐릭터는 첫 게임 입장에서 초기 진행을
    만들지 못한다. 클라이언트의 마법사 버튼이 그 경로다. 이름이 DB 열 한도를 넘으면 생성이
    내부 오류로 끝난다.

    픽스처 결합도: 생성할 수 있는 직업을 LobbyData::Install로 주입한다. 게임의 기획 데이터는 읽지 않는다.
    NONE도 생성할 수 있는 직업에 넣어, 표에 있어도 NONE이 막히는지 본다.
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
        LobbyTables tables;
        tables.creatableClasses = { Protocol::CLASS_TYPE_NONE, Protocol::CLASS_TYPE_WARRIOR };
        LobbyData::Install(std::move(tables));
    }

    void TearDown() override
    {
        LobbyData::Install(LobbyTables());
    }
};

TEST_F(CharacterCreationTest, WarriorWithValidNamePasses)
{
    EXPECT_FALSE(CharacterCreation::Validate(MakeCharacter(Protocol::CLASS_TYPE_WARRIOR, "전사")).has_value());
}

TEST_F(CharacterCreationTest, ClassNotCreatableInLobbyIsRejected)
{
    const auto cause = CharacterCreation::Validate(MakeCharacter(Protocol::CLASS_TYPE_MAGE, "마법사"));

    ASSERT_TRUE(cause.has_value()) << "생성할 수 없는 직업을 통과시키면 그 캐릭터는 첫 입장에서 초기 진행을 만들지 못한다";
    EXPECT_EQ(cause.value(), "선택할 수 없는 직업입니다.");
}

TEST_F(CharacterCreationTest, NoneClassIsRejectedEvenWhenListedAsCreatable)
{
    EXPECT_TRUE(CharacterCreation::Validate(MakeCharacter(Protocol::CLASS_TYPE_NONE, "무직")).has_value())
        << "NONE은 직업이 아니다. 누가 생성할 수 있는 직업에 넣어도 통과하면 안 된다";
}

TEST_F(CharacterCreationTest, UndefinedClassNumberIsRejected)
{
    EXPECT_TRUE(CharacterCreation::Validate(MakeCharacter(99, "정의안됨")).has_value());
}

TEST_F(CharacterCreationTest, EmptyNameIsRejected)
{
    const auto cause = CharacterCreation::Validate(MakeCharacter(Protocol::CLASS_TYPE_WARRIOR, ""));

    ASSERT_TRUE(cause.has_value());
    EXPECT_EQ(cause.value(), "캐릭터 이름은 32이하여야 합니다.");
}

TEST_F(CharacterCreationTest, ThirtyTwoHangulCharactersPass)
{
    EXPECT_FALSE(CharacterCreation::Validate(MakeCharacter(Protocol::CLASS_TYPE_WARRIOR, MakeHangulName(32))).has_value())
        << "UTF-8 바이트(96)가 아니라 UTF-16 글자 수(32)로 세야 한다";
}

TEST_F(CharacterCreationTest, ThirtyThreeCharactersAreRejected)
{
    EXPECT_TRUE(CharacterCreation::Validate(MakeCharacter(Protocol::CLASS_TYPE_WARRIOR, MakeHangulName(33))).has_value())
        << "DB 열 한도(NVARCHAR(32))를 넘으면 생성 쿼리가 내부 오류로 끝난다";
}
