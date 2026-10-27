#include "Core/pch.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "FakeDBConnection.h"
#include "DB/DAOCommon.h"
#include "Lobby/CharacterListDAO.h"
#include "Lobby/LobbyRequests.h"
#include "Lobby/Data/LobbyData.h"

/*--------------------------------------------------------------
    로비 요청 테스트

    로비의 응답은 실패를 화면 문구가 아니라 로비 결과 값으로 싣는다. 클라이언트가 결과 값을 문구로 바꾸므로,
    서버가 문구를 보내던 때처럼 사유가 문자열 비교로만 갈리지 않는다.

    로비는 배선과 게임 입장 창구만 부른다. 같은 계정의 앞 연결을 밀어내는 일은 세션 관리자가 맡으므로,
    로비 코드는 진행 조율자를 include하지 않는다.

    이름과 직업의 검사는 핸들러가 DB 큐로 넘기기 전에 한다. 그 결과 값은 CharacterCreationTest와 PacketHandlerTest가 본다.

    픽스처 결합도: FakeDBConnection과 LobbyData::Install로 넣은 로비 설정을 쓴다. 생성할 수 있는 직업은 전사뿐이다.
    SQL 문장과 거절 판정은 DAO 테스트와 실제 DB 테스트가 본다. 마지막 테스트는 저장소의 Lobby/ 폴더를 읽는다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 ACCOUNT_ID = 7;
    constexpr int64 CHARACTER_ID = 5'000'000'000;
    constexpr int32 SLOT_COUNT = 4;

    Protocol::C_CREATE_CHARACTER MakeCreate(const string& name, Protocol::CharacterClass characterClass)
    {
        Protocol::C_CREATE_CHARACTER pkt;
        pkt.set_name(name);
        pkt.set_character_class(characterClass);
        return pkt;
    }
}

class LobbyRequestsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        LobbyDesignData::LobbyConfig config;
        config.DefaultCharacterSlotCount = SLOT_COUNT;
        config.CreatableClasses = { Protocol::CHARACTER_CLASS_WARRIOR };
        LobbyData::Install(std::move(config));
    }

    void TearDown() override
    {
        LobbyData::Install(LobbyDesignData::LobbyConfig());
    }

    FakeDBConnection conn;
};

TEST_F(LobbyRequestsTest, ConnectLobbyListsCharactersWithSlotCountOfAccountRow)
{
    conn.QueueResult({ { int64(6) } });
    conn.QueueResult({
        { CHARACTER_ID, int64(Protocol::CHARACTER_CLASS_WARRIOR), wstring(L"전사"), int64(12) },
        { CHARACTER_ID + 1, int64(Protocol::CHARACTER_CLASS_MAGE), wstring(L"마법사"), int64(3) },
    });

    const Protocol::S_CHARACTER_LIST listPkt = LobbyRequests::ConnectLobby(conn, ACCOUNT_ID);

    EXPECT_EQ(listPkt.character_slot_count(), 6) << "슬롯 수는 로비 설정이 아니라 계정 행의 값이다";
    ASSERT_EQ(listPkt.characters_size(), 2);
    EXPECT_EQ(listPkt.characters(0).character_id(), CHARACTER_ID);
    EXPECT_EQ(listPkt.characters(0).name(), "전사");
    EXPECT_EQ(listPkt.characters(0).character_class(), Protocol::CHARACTER_CLASS_WARRIOR);
    EXPECT_EQ(listPkt.characters(0).level(), 12);
    EXPECT_EQ(listPkt.characters(1).character_class(), Protocol::CHARACTER_CLASS_MAGE)
        << "목록은 지금 생성할 수 없는 직업의 캐릭터도 보여 준다";
}

TEST_F(LobbyRequestsTest, ConnectLobbyCarriesCreatableClassesOfLobbyTable)
{
    LobbyDesignData::LobbyConfig config;
    config.DefaultCharacterSlotCount = SLOT_COUNT;
    config.CreatableClasses = { Protocol::CHARACTER_CLASS_MAGE, Protocol::CHARACTER_CLASS_WARRIOR };
    LobbyData::Install(std::move(config));
    conn.QueueResult({ { int64(SLOT_COUNT) } });
    conn.QueueResult({});

    const Protocol::S_CHARACTER_LIST listPkt = LobbyRequests::ConnectLobby(conn, ACCOUNT_ID);

    ASSERT_EQ(listPkt.creatable_classes_size(), 2) << "클라이언트는 직업 목록을 따로 갖지 않고 이 값만 보여 준다";
    EXPECT_EQ(listPkt.creatable_classes(0), Protocol::CHARACTER_CLASS_MAGE) << "로비 설정의 순서를 그대로 싣는다";
    EXPECT_EQ(listPkt.creatable_classes(1), Protocol::CHARACTER_CLASS_WARRIOR);
}

TEST_F(LobbyRequestsTest, CreatedCharacterComesBackAsLobbyCharacter)
{
    conn.QueueResult({ { int64(0) } });

    const Protocol::S_CREATE_CHARACTER createPkt = LobbyRequests::CreateCharacter(conn, ACCOUNT_ID, CHARACTER_ID, MakeCreate("전사", Protocol::CHARACTER_CLASS_WARRIOR));

    EXPECT_EQ(createPkt.result(), Protocol::LOBBY_RESULT_SUCCESS);
    ASSERT_TRUE(createPkt.has_character()) << "클라이언트는 응답의 캐릭터를 목록에 더한다";
    EXPECT_EQ(createPkt.character().character_id(), CHARACTER_ID);
    EXPECT_EQ(createPkt.character().name(), "전사");
    EXPECT_EQ(createPkt.character().character_class(), Protocol::CHARACTER_CLASS_WARRIOR);
    EXPECT_EQ(createPkt.character().level(), 1) << "새 캐릭터는 1레벨에서 시작한다";
}

TEST_F(LobbyRequestsTest, DuplicateNameIsNameTaken)
{
    conn.QueueResult({ { int64(CharacterRejection::DUPLICATE_NAME) } });

    const Protocol::S_CREATE_CHARACTER createPkt = LobbyRequests::CreateCharacter(conn, ACCOUNT_ID, CHARACTER_ID, MakeCreate("전사", Protocol::CHARACTER_CLASS_WARRIOR));

    EXPECT_EQ(createPkt.result(), Protocol::LOBBY_RESULT_NAME_TAKEN);
    EXPECT_FALSE(createPkt.has_character()) << "캐릭터는 성공했을 때만 싣는다";
}

TEST_F(LobbyRequestsTest, FullSlotsAreNoEmptySlot)
{
    conn.QueueResult({ { int64(CharacterRejection::NO_EMPTY_SLOT) } });

    const Protocol::S_CREATE_CHARACTER createPkt = LobbyRequests::CreateCharacter(conn, ACCOUNT_ID, CHARACTER_ID, MakeCreate("전사", Protocol::CHARACTER_CLASS_WARRIOR));

    EXPECT_EQ(createPkt.result(), Protocol::LOBBY_RESULT_NO_EMPTY_SLOT);
    EXPECT_FALSE(createPkt.has_character());
}

TEST_F(LobbyRequestsTest, DeletedCharacterIsSuccessWithRequestedId)
{
    conn.QueueResult({}, 1);

    const Protocol::S_DELETE_CHARACTER deletePkt = LobbyRequests::DeleteCharacter(conn, ACCOUNT_ID, CHARACTER_ID);

    EXPECT_EQ(deletePkt.result(), Protocol::LOBBY_RESULT_SUCCESS);
    EXPECT_EQ(deletePkt.character_id(), CHARACTER_ID);
}

TEST_F(LobbyRequestsTest, DeletingOtherAccountCharacterIsCharacterNotFound)
{
    conn.QueueResult({}, 0);

    const Protocol::S_DELETE_CHARACTER deletePkt = LobbyRequests::DeleteCharacter(conn, ACCOUNT_ID, CHARACTER_ID);

    EXPECT_EQ(deletePkt.result(), Protocol::LOBBY_RESULT_CHARACTER_NOT_FOUND);
    EXPECT_EQ(deletePkt.character_id(), CHARACTER_ID) << "실패해도 요청한 캐릭터 번호를 싣는다";
}

TEST_F(LobbyRequestsTest, SelectingOwnCharacterSucceeds)
{
    conn.QueueResult({ { int64(1) } });

    EXPECT_EQ(LobbyRequests::SelectCharacter(conn, ACCOUNT_ID, CHARACTER_ID), Protocol::LOBBY_RESULT_SUCCESS);
}

// 캐릭터 번호는 클라이언트 값이다. 다른 계정의 캐릭터를 고르면 게임 입장 창구로 넘기지 않는다.
TEST_F(LobbyRequestsTest, SelectingOtherAccountCharacterIsCharacterNotFound)
{
    conn.QueueResult({ { int64(0) } });

    EXPECT_EQ(LobbyRequests::SelectCharacter(conn, ACCOUNT_ID, CHARACTER_ID), Protocol::LOBBY_RESULT_CHARACTER_NOT_FOUND);
}

TEST_F(LobbyRequestsTest, FailedQueryIsDBError)
{
    conn.QueueExecuteFailure();

    EXPECT_THROW(LobbyRequests::DeleteCharacter(conn, ACCOUNT_ID, CHARACTER_ID), DBError)
        << "DB 작업의 실패는 거절이 아니다. 핸들러가 내부 오류로 응답한다";
}

// 로비가 진행 조율자를 부르면 로비와 게임이 진행 저장의 순서를 함께 알게 된다. 밀어내기는 세션 관리자의 일이다.
TEST(LobbyModuleTest, LobbyDoesNotIncludeProgressCoordinator)
{
    const filesystem::path lobbyDir = filesystem::path(__FILE__).parent_path() / ".." / "GameServer" / "Lobby";
    ASSERT_TRUE(filesystem::is_directory(lobbyDir)) << "로비 폴더를 찾지 못했다: " << lobbyDir.string();

    int32 checkedFiles = 0;
    for (const filesystem::directory_entry& entry : filesystem::recursive_directory_iterator(lobbyDir))
    {
        const string extension = entry.path().extension().string();
        if (extension != ".h" && extension != ".cpp")
            continue;

        ifstream file(entry.path());
        const string source((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        EXPECT_EQ(source.find("ProgressCoordinator.h"), string::npos) << entry.path().string();
        checkedFiles++;
    }

    EXPECT_GT(checkedFiles, 0) << "검사한 파일이 없으면 아무것도 검사하지 않은 것이다";
}

// 로비와 게임은 서로의 기획 데이터를 읽지 않는다. 로비를 별도 프로세스로 옮길 때 게임의 산출물이 따라오지 않게 한다.
TEST(LobbyModuleTest, LobbyDoesNotIncludeGameDesignData)
{
    const filesystem::path lobbyDir = filesystem::path(__FILE__).parent_path() / ".." / "GameServer" / "Lobby";
    ASSERT_TRUE(filesystem::is_directory(lobbyDir)) << "로비 폴더를 찾지 못했다: " << lobbyDir.string();

    int32 checkedFiles = 0;
    for (const filesystem::directory_entry& entry : filesystem::recursive_directory_iterator(lobbyDir))
    {
        const string extension = entry.path().extension().string();
        if (extension != ".h" && extension != ".cpp")
            continue;

        ifstream file(entry.path());
        const string source((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        EXPECT_EQ(source.find("\"Game/Data/"), string::npos) << entry.path().string();
        checkedFiles++;
    }

    EXPECT_GT(checkedFiles, 0) << "검사한 파일이 없으면 아무것도 검사하지 않은 것이다";
}

// 로비 복귀의 목록은 로비 접속의 목록과 같은 내용이다. 계정 행은 이미 있으므로 로그인을 기록하지 않고 슬롯 수만 읽는다.
TEST_F(LobbyRequestsTest, ReturnToLobbyListsCharactersWithSlotCountOfAccountRow)
{
    conn.QueueResult({ { int64(6) } });
    conn.QueueResult({
        { CHARACTER_ID, int64(Protocol::CHARACTER_CLASS_WARRIOR), wstring(L"전사"), int64(13) },
    });

    const Protocol::S_CHARACTER_LIST listPkt = LobbyRequests::ReturnToLobby(conn, ACCOUNT_ID);

    EXPECT_EQ(listPkt.character_slot_count(), 6);
    ASSERT_EQ(listPkt.characters_size(), 1);
    EXPECT_EQ(listPkt.characters(0).level(), 13);
    ASSERT_EQ(listPkt.creatable_classes_size(), 1);
    EXPECT_EQ(listPkt.creatable_classes(0), Protocol::CHARACTER_CLASS_WARRIOR);
    ASSERT_EQ(conn.executedParams.size(), 2u);
    EXPECT_EQ(conn.executedParams[0].size(), 1u) << "슬롯 수 조회는 계정 번호만 바인딩한다. 로그인 기록은 기본 슬롯 수까지 둘이다";
}
