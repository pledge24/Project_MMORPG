#include "Core/pch.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "Core/DataCrossCheck.h"
#include "Lobby/Data/LobbyData.h"

/*--------------------------------------------------------------
    로비 기획 데이터와 기동할 때의 대조 테스트

    로비 표는 새 계정의 슬롯 수와 생성할 수 있는 직업을 정한다. 로비와 게임은 서로의 기획 데이터를 읽지 않으므로,
    생성할 수 있는 직업마다 게임의 레벨 표가 있는지는 서버가 기동할 때 배선이 대조한다. 레벨 표가 없는 직업의 캐릭터는
    만들 수는 있어도 첫 게임 입장에서 초기 진행을 만들지 못해 들어가지 못한다.

    픽스처 결합도: 문서를 Json으로 손으로 만들고, 레벨 표는 Gamedata::Install로 주입한다.
---------------------------------------------------------------*/

namespace
{
    Json MakeLobbyRow(int32 slotCount, Json classes)
    {
        return Json{ {"defaultCharacterSlotCount", slotCount}, {"creatableClasses", std::move(classes)} };
    }

    Json MakeLobbyDocument(int32 slotCount, Json classes)
    {
        return Json::array({ MakeLobbyRow(slotCount, std::move(classes)) });
    }

    void ExpectRejected(const Json& document, string_view mention)
    {
        const optional<string> error = LobbyData::Load(document);
        ASSERT_TRUE(error.has_value()) << document.dump();
        EXPECT_NE(error->find("S_Lobby.json"), string::npos) << error.value();
        EXPECT_NE(error->find(mention), string::npos) << error.value();
    }
}

class LobbyDataTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        LobbyData::Install(LobbyTables());
    }
};

TEST_F(LobbyDataTest, RowBecomesSlotCountAndCreatableClasses)
{
    ASSERT_FALSE(LobbyData::Load(MakeLobbyDocument(4, Json::array({ "warrior", "mage" }))).has_value());

    EXPECT_EQ(LobbyData::GetDefaultCharacterSlotCount(), 4);
    EXPECT_EQ(LobbyData::GetCreatableClasses(), (vector<int32>{ Protocol::CLASS_TYPE_WARRIOR, Protocol::CLASS_TYPE_MAGE }));
    EXPECT_TRUE(LobbyData::IsCreatableClass(Protocol::CLASS_TYPE_MAGE));
    EXPECT_FALSE(LobbyData::IsCreatableClass(Protocol::CLASS_TYPE_NONE));
}

// DB의 Account.CharacterSlotCount가 TINYINT라 255를 넘으면 계정을 만들 때 저장하지 못한다.
TEST_F(LobbyDataTest, SlotCountOutOfRangeIsRejected)
{
    for (int32 slotCount : { 0, 256 })
    {
        SCOPED_TRACE(slotCount);
        ExpectRejected(MakeLobbyDocument(slotCount, Json::array({ "warrior" })), "defaultCharacterSlotCount");
    }

    EXPECT_FALSE(LobbyData::Load(MakeLobbyDocument(255, Json::array({ "warrior" }))).has_value()) << "255는 저장할 수 있다";
}

TEST_F(LobbyDataTest, CreatableClassesMustBeKnownNonEmptyAndUnique)
{
    ExpectRejected(MakeLobbyDocument(4, Json::array()), "creatableClasses");
    ExpectRejected(MakeLobbyDocument(4, Json::array({ "rogue" })), "rogue");
    ExpectRejected(MakeLobbyDocument(4, Json::array({ "warrior", "warrior" })), "warrior");
    ExpectRejected(MakeLobbyDocument(4, Json::array({ 1 })), "creatableClasses");
}

TEST_F(LobbyDataTest, TableMustHaveExactlyOneRow)
{
    ExpectRejected(Json::array(), "S_Lobby.json");

    Json twoRows = MakeLobbyDocument(4, Json::array({ "warrior" }));
    twoRows.push_back(MakeLobbyRow(4, Json::array({ "warrior" })));
    ExpectRejected(twoRows, "S_Lobby.json");
}

TEST_F(LobbyDataTest, FailedLoadKeepsPreviousTable)
{
    ASSERT_FALSE(LobbyData::Load(MakeLobbyDocument(4, Json::array({ "warrior" }))).has_value());

    EXPECT_TRUE(LobbyData::Load(MakeLobbyDocument(0, Json::array({ "mage" }))).has_value());

    EXPECT_EQ(LobbyData::GetDefaultCharacterSlotCount(), 4) << "실패한 불러오기가 이전 표를 지웠다";
    EXPECT_FALSE(LobbyData::IsCreatableClass(Protocol::CLASS_TYPE_MAGE)) << "실패한 불러오기가 일부 값을 설치했다";
}

TEST_F(LobbyDataTest, RepositoryLobbyDataPasses)
{
    // 테스트 소스 위치에서 GameServer의 로비 표를 찾는다. 작업 디렉터리와 무관하게 돈다.
    const filesystem::path path = filesystem::path(__FILE__).parent_path() / ".." / "GameServer" / "Lobby" / "Data" / "Json" / "S_Lobby.json";
    ifstream file(path);
    ASSERT_TRUE(file.is_open()) << "로비 표를 찾지 못했다: " << path.string();

    const optional<string> error = LobbyData::Load(Json::parse(file));
    EXPECT_FALSE(error.has_value()) << error.value_or("");

    // 옛 캐릭터 생성 표에서 옮겨 온 값이다. 옮기면서 값이 바뀌지 않았는지 본다.
    EXPECT_EQ(LobbyData::GetDefaultCharacterSlotCount(), 4);
    EXPECT_EQ(LobbyData::GetCreatableClasses(), (vector<int32>{ Protocol::CLASS_TYPE_WARRIOR }))
        << "레벨 표가 있는 직업은 전사뿐이다";
}

/* 기동할 때의 대조 */

class DataCrossCheckTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ LevelTemplate{ 1, 100, 50 } });
        Gamedata::Install(std::move(tables));
    }

    void TearDown() override
    {
        Gamedata::Install(GamedataTables());
        LobbyData::Install(LobbyTables());
    }

    static void InstallCreatableClasses(vector<int32> classes)
    {
        LobbyTables tables;
        tables.defaultCharacterSlotCount = 4;
        tables.creatableClasses = std::move(classes);
        LobbyData::Install(std::move(tables));
    }
};

TEST_F(DataCrossCheckTest, CreatableClassesWithLevelTablesPass)
{
    InstallCreatableClasses({ Protocol::CLASS_TYPE_WARRIOR });

    const optional<string> error = DataCrossCheck::Check();
    EXPECT_FALSE(error.has_value()) << error.value_or("");
}

TEST_F(DataCrossCheckTest, CreatableClassWithoutLevelTableIsRejected)
{
    InstallCreatableClasses({ Protocol::CLASS_TYPE_WARRIOR, Protocol::CLASS_TYPE_MAGE });

    const optional<string> error = DataCrossCheck::Check();
    ASSERT_TRUE(error.has_value()) << "레벨 표가 없는 직업을 만들게 두면 그 캐릭터는 첫 입장에서 들어가지 못한다";
    EXPECT_NE(error->find(to_string(Protocol::CLASS_TYPE_MAGE)), string::npos) << error.value();
}
