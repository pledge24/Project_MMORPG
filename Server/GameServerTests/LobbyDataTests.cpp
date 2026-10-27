#include "Core/pch.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "Core/DataCrossCheck.h"
#include "Lobby/Data/LobbyData.h"
#include "TestLevelTables.h"

/*--------------------------------------------------------------
    로비 기획 데이터와 기동할 때의 대조 테스트

    로비 설정은 새 계정의 슬롯 수와 생성할 수 있는 직업을 정한다. 값의 규칙(슬롯 수 1 이상, 직업 목록이 비지 않고
    겹치지 않음)은 DataBuilder가 검사하고, 서버는 DB 열 크기만 검사한다. 로비와 게임은 서로의 기획 데이터를 읽지 않으므로,
    생성할 수 있는 직업마다 게임의 직업별 능력치 표가 있는지는 서버가 기동할 때 배선이 대조한다.

    픽스처 결합도: 문서를 Json으로 손으로 만들고, 능력치 표는 Gamedata::Install로 주입한다.
---------------------------------------------------------------*/

namespace
{
    const filesystem::path GAME_SERVER_DIR = filesystem::path(__FILE__).parent_path() / ".." / "GameServer";

    Json MakeLobbyDocument(int32 slotCount, Json classes)
    {
        return Json{ {"DefaultCharacterSlotCount", slotCount}, {"CreatableClasses", std::move(classes)} };
    }

    void ExpectRejected(const Json& document, string_view mention)
    {
        const optional<string> error = LobbyData::Load(document);
        ASSERT_TRUE(error.has_value()) << document.dump();
        EXPECT_NE(error->find(LobbyDesignData::LOBBY_FILE), string::npos) << error.value();
        EXPECT_NE(error->find(mention), string::npos) << error.value();
    }
}

class LobbyDataTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        LobbyData::Install(LobbyDesignData::LobbyConfig());
    }
};

TEST_F(LobbyDataTest, DocumentBecomesSlotCountAndCreatableClasses)
{
    ASSERT_FALSE(LobbyData::Load(MakeLobbyDocument(4, Json::array({ "WARRIOR", "MAGE" }))).has_value());

    EXPECT_EQ(LobbyData::GetDefaultCharacterSlotCount(), 4);
    EXPECT_EQ(LobbyData::GetCreatableClasses(), (vector<Protocol::CharacterClass>{ Protocol::CHARACTER_CLASS_WARRIOR, Protocol::CHARACTER_CLASS_MAGE }));
    EXPECT_TRUE(LobbyData::IsCreatableClass(Protocol::CHARACTER_CLASS_MAGE));
    EXPECT_FALSE(LobbyData::IsCreatableClass(Protocol::CHARACTER_CLASS_NONE));
}

// DB의 Account.CharacterSlotCount가 TINYINT라 255를 넘으면 계정을 만들 때 저장하지 못한다.
TEST_F(LobbyDataTest, SlotCountAboveTinyIntIsRejected)
{
    ExpectRejected(MakeLobbyDocument(256, Json::array({ "WARRIOR" })), "DefaultCharacterSlotCount");

    EXPECT_FALSE(LobbyData::Load(MakeLobbyDocument(255, Json::array({ "WARRIOR" }))).has_value()) << "255는 저장할 수 있다";
}

TEST_F(LobbyDataTest, MalformedDocumentIsRejected)
{
    ExpectRejected(Json{ {"DefaultCharacterSlotCount", 4} }, "CreatableClasses");
    ExpectRejected(MakeLobbyDocument(4, Json::array({ "ROGUE" })), "CreatableClasses");
    ExpectRejected(Json::array(), LobbyDesignData::LOBBY_FILE);
}

TEST_F(LobbyDataTest, FailedLoadKeepsPreviousConfig)
{
    ASSERT_FALSE(LobbyData::Load(MakeLobbyDocument(4, Json::array({ "WARRIOR" }))).has_value());

    EXPECT_TRUE(LobbyData::Load(MakeLobbyDocument(256, Json::array({ "MAGE" }))).has_value());

    EXPECT_EQ(LobbyData::GetDefaultCharacterSlotCount(), 4) << "실패한 불러오기가 이전 설정을 지웠다";
    EXPECT_FALSE(LobbyData::IsCreatableClass(Protocol::CHARACTER_CLASS_MAGE)) << "실패한 불러오기가 일부 값을 설치했다";
}

TEST_F(LobbyDataTest, RepositoryLobbyDataPasses)
{
    // 테스트 소스 위치에서 GameServer의 로비 설정을 찾는다. 작업 디렉터리와 무관하게 돈다.
    const filesystem::path path = GAME_SERVER_DIR / "Lobby" / "Data" / "Generated" / LobbyDesignData::LOBBY_FILE;
    ifstream file(path);
    ASSERT_TRUE(file.is_open()) << "로비 설정을 찾지 못했다: " << path.string();

    const optional<string> error = LobbyData::Load(Json::parse(file));
    EXPECT_FALSE(error.has_value()) << error.value_or("");

    // 옛 로비 표에서 옮겨 온 값이다. 옮기면서 값이 바뀌지 않았는지 본다.
    EXPECT_EQ(LobbyData::GetDefaultCharacterSlotCount(), 4);
    EXPECT_EQ(LobbyData::GetCreatableClasses(), (vector<Protocol::CharacterClass>{ Protocol::CHARACTER_CLASS_WARRIOR }))
        << "직업별 능력치 표가 있는 직업은 전사뿐이다";
}

/* 기동할 때의 대조 */

class DataCrossCheckTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        DesignData::ClassLevelStatRow& level1 = TestLevelTables::AddWarriorLevel(tables, 1);
        level1.MaxHp = 100;
        level1.MaxMp = 50;
        Gamedata::Install(std::move(tables));
    }

    void TearDown() override
    {
        Gamedata::Install(GamedataTables());
        LobbyData::Install(LobbyDesignData::LobbyConfig());
    }

    static void InstallCreatableClasses(vector<Protocol::CharacterClass> classes)
    {
        LobbyDesignData::LobbyConfig config;
        config.DefaultCharacterSlotCount = 4;
        config.CreatableClasses = std::move(classes);
        LobbyData::Install(std::move(config));
    }
};

TEST_F(DataCrossCheckTest, CreatableClassesWithClassLevelStatsPass)
{
    InstallCreatableClasses({ Protocol::CHARACTER_CLASS_WARRIOR });

    const optional<string> error = DataCrossCheck::Check();
    EXPECT_FALSE(error.has_value()) << error.value_or("");
}

TEST_F(DataCrossCheckTest, CreatableClassWithoutClassLevelStatsIsRejected)
{
    InstallCreatableClasses({ Protocol::CHARACTER_CLASS_WARRIOR, Protocol::CHARACTER_CLASS_MAGE });

    const optional<string> error = DataCrossCheck::Check();
    ASSERT_TRUE(error.has_value()) << "능력치 표가 없는 직업을 만들게 두면 그 캐릭터는 첫 입장에서 들어가지 못한다";
    EXPECT_NE(error->find("CHARACTER_CLASS_MAGE"), string::npos) << error.value();
}
