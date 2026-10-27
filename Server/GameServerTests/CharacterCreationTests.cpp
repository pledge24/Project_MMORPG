#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Lobby/CharacterCreation.h"
#include "Utils/EncodingConverter.h"
#include "Lobby/Data/LobbyData.h"

/*--------------------------------------------------------------
    캐릭터 생성 검증 테스트

    거절은 화면 문구가 아니라 로비 결과 값으로 돌려준다. 클라이언트가 결과 값을 문구로 바꾼다.

    로비 설정의 생성할 수 있는 직업이 아니면 거절한다. 그런 직업의 캐릭터는 첫 게임 입장에서 초기 진행을
    만들지 못한다. 클라이언트의 마법사 버튼이 그 경로다. 이름이 DB 열 한도를 넘으면 생성이
    내부 오류로 끝난다.

    픽스처 결합도: 생성할 수 있는 직업을 LobbyData::Install로 주입한다. 게임의 기획 데이터는 읽지 않는다.
    NONE도 생성할 수 있는 직업에 넣어, 표에 있어도 NONE이 막히는지 본다.
---------------------------------------------------------------*/

namespace
{
    optional<Protocol::LobbyResult> Validate(int32 classId, const string& name)
    {
        return CharacterCreation::Validate(name, static_cast<Protocol::CharacterClass>(classId));
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
        LobbyDesignData::LobbyConfig config;
        config.CreatableClasses = { Protocol::CHARACTER_CLASS_NONE, Protocol::CHARACTER_CLASS_WARRIOR };
        LobbyData::Install(std::move(config));
    }

    void TearDown() override
    {
        LobbyData::Install(LobbyDesignData::LobbyConfig());
    }
};

TEST_F(CharacterCreationTest, WarriorWithValidNamePasses)
{
    EXPECT_FALSE(Validate(Protocol::CHARACTER_CLASS_WARRIOR, "전사").has_value());
}

TEST_F(CharacterCreationTest, ClassNotCreatableInLobbyIsRejected)
{
    const auto result = Validate(Protocol::CHARACTER_CLASS_MAGE, "마법사");

    ASSERT_TRUE(result.has_value()) << "생성할 수 없는 직업을 통과시키면 그 캐릭터는 첫 입장에서 초기 진행을 만들지 못한다";
    EXPECT_EQ(result.value(), Protocol::LOBBY_RESULT_UNAVAILABLE_CLASS);
}

TEST_F(CharacterCreationTest, NoneClassIsRejectedEvenWhenListedAsCreatable)
{
    EXPECT_EQ(Validate(Protocol::CHARACTER_CLASS_NONE, "무직"), Protocol::LOBBY_RESULT_UNAVAILABLE_CLASS)
        << "NONE은 직업이 아니다. 누가 생성할 수 있는 직업에 넣어도 통과하면 안 된다";
}

TEST_F(CharacterCreationTest, UndefinedClassNumberIsRejected)
{
    EXPECT_EQ(Validate(99, "정의안됨"), Protocol::LOBBY_RESULT_UNAVAILABLE_CLASS);
}

TEST_F(CharacterCreationTest, EmptyNameIsRejected)
{
    EXPECT_EQ(Validate(Protocol::CHARACTER_CLASS_WARRIOR, ""), Protocol::LOBBY_RESULT_INVALID_NAME);
}

TEST_F(CharacterCreationTest, ThirtyTwoHangulCharactersPass)
{
    EXPECT_FALSE(Validate(Protocol::CHARACTER_CLASS_WARRIOR, MakeHangulName(32)).has_value())
        << "UTF-8 바이트(96)가 아니라 UTF-16 글자 수(32)로 세야 한다";
}

TEST_F(CharacterCreationTest, ThirtyThreeCharactersAreRejected)
{
    EXPECT_EQ(Validate(Protocol::CHARACTER_CLASS_WARRIOR, MakeHangulName(33)), Protocol::LOBBY_RESULT_INVALID_NAME)
        << "DB 열 한도(NVARCHAR(32))를 넘으면 생성 쿼리가 내부 오류로 끝난다";
}
