#include "Core/pch.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "Game/Data/GamedataParser.h"

/*--------------------------------------------------------------
    기획 데이터 변환 테스트

    기획표의 형식 오류는 부팅할 때 드러나야 한다. 게임 중에 Json을 읽다가 틀린 행을
    만나면 룸 스레드에서 예외가 터지거나 조용히 기본값으로 동작한다. 부팅에서 걸러 낸
    오류는 어느 파일의 몇 번째 행, 어느 필드인지를 알려야 기획자가 고칠 수 있다.

    마지막 테스트는 저장소의 실제 기획표가 이 검증을 통과하는지 본다. 검증을 조이면서
    실제 데이터가 서버를 못 띄우게 되는 일을 여기서 잡는다.

    픽스처 결합도: 없음. Gamedata::Load를 부르는 테스트만 전역 표를 바꾸고 끝날 때 비운다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 SWORD_ID = 1005;
    constexpr int32 POTION_ID = 2000;
    constexpr int32 MONSTER_ID = 5000;
    constexpr int32 TOWN_ID = 10;
    constexpr int32 FIELD_ID = 20;

    Json MakeLevelRow(int32 level)
    {
        return Json{
            {"level", level}, {"maxHp", 500 + level}, {"maxMp", 100}, {"physicalAttack", 10}, {"magicalAttack", 2},
            {"expRequirement", 50 * level},
            {"maxHpIncrement", 100}, {"maxMpIncrement", 100}, {"paIncrement", 10}, {"maIncrement", 2},
        };
    }

    Json MakeSword()
    {
        return Json{
            {"templateId", SWORD_ID}, {"itemName", "초보자의 검"}, {"itemType", "GEAR"}, {"itemSubtype", "sword"},
            {"levelRequirement", 10}, {"classRequirement", "warrior"},
            {"buyPrice", 100}, {"sellPrice", 10}, {"sellable", true}, {"stackable", false}, {"maxStack", 1},
            {"cooldown", -1}, {"physicalAttack", 15}, {"magicalAttack", 0}, {"hp", 0}, {"mp", 0},
        };
    }

    Json MakePotion()
    {
        return Json{
            {"templateId", POTION_ID}, {"itemName", "HP 포션"}, {"itemType", "CONSUMABLE"}, {"itemSubtype", "potion"},
            {"levelRequirement", 0}, {"classRequirement", "all"},
            {"buyPrice", 50}, {"sellPrice", 5}, {"sellable", true}, {"stackable", true}, {"maxStack", 99},
            {"cooldown", 10}, {"hpRestore", 0.3}, {"mpRestore", 0},
        };
    }

    Json MakeMonster()
    {
        return Json{
            {"templateId", MONSTER_ID}, {"level", 3}, {"monsterName", "미니언"}, {"attackType", "melee"},
            {"maxHp", 200}, {"baseAttack", 100}, {"movementSpeed", 300}, {"isTargeting", true},
            {"expReward", {{"minExp", 5}, {"maxExp", 10}}},
            {"goldReward", {{"minGold", 10}, {"maxGold", 20}}},
            {"attackInterval", 2.5}, {"tryAttackRange", 150}, {"detectionRange", 900}, {"chasingMaxRange", 1200},
        };
    }

    Json MakePos(float x, float y, float z)
    {
        return Json{{"posX", x}, {"posY", y}, {"posZ", z}};
    }

    Json MakeTown()
    {
        Json portal = {
            {"portalId", 11}, {"type", "room"}, {"src", MakePos(-1200, 1000, 0)},
            {"dst", {{"templateId", FIELD_ID}, {"posX", -8000}, {"posY", 10000}, {"posZ", 0}, {"yaw", 180.0}}},
        };

        return Json{
            {"templateId", TOWN_ID}, {"mapId", 1111}, {"mapName", "마을"}, {"mapType", "safe zone"},
            {"hasRespawnPoint", true}, {"centerPos", MakePos(0, 0, 0)},
            {"widthHalfExtent", 1500}, {"depthHalfExtent", 1500}, {"respawnPoint", MakePos(10, 20, 0)},
            {"portals", {{"lists", Json::array({portal})}}},
            {"monsterIds", Json::array()}, {"maxMonsterCount", nullptr}, {"monsterRespawnTime", nullptr},
            {"portalRadius", 500},
        };
    }

    Json MakeField()
    {
        return Json{
            {"templateId", FIELD_ID}, {"mapId", 1111}, {"mapName", "사냥터"}, {"mapType", "hunting field"},
            {"hasRespawnPoint", false}, {"centerPos", MakePos(-10000, 10000, 0)},
            {"widthHalfExtent", 2000}, {"depthHalfExtent", 2000}, {"respawnPoint", nullptr},
            {"portals", {{"lists", Json::array()}}},
            {"monsterIds", Json::array({MONSTER_ID})}, {"maxMonsterCount", 10}, {"monsterRespawnTime", 10},
            {"portalRadius", 300.5},
        };
    }

    GamedataDocuments MakeValidDocuments()
    {
        GamedataDocuments documents;
        documents.warriorLevels = Json::array({MakeLevelRow(1), MakeLevelRow(2), MakeLevelRow(3)});
        documents.items = Json::array({MakeSword(), MakePotion()});
        documents.monsters = Json::array({MakeMonster()});
        documents.maps = Json::array({MakeTown(), MakeField()});
        return documents;
    }

    /** 실패해야 하는 문서를 넣고 오류 메시지를 돌려준다. 통과하면 빈 문자열이다. */
    string ErrorOf(const GamedataDocuments& documents)
    {
        GamedataTables tables;
        return GamedataParser::Parse(documents, OUT tables).value_or("");
    }

    void ExpectMentions(const string& error, initializer_list<string_view> parts)
    {
        ASSERT_FALSE(error.empty()) << "틀린 행이 통과했다";
        for (string_view part : parts)
            EXPECT_NE(error.find(part), string::npos) << "오류 메시지에 '" << part << "'이 없다: " << error;
    }

    Json ReadJsonFile(const filesystem::path& path)
    {
        ifstream file(path);
        if (file.is_open() == false)
            return Json();

        return Json::parse(file);
    }
}

TEST(GamedataParserTest, ValidDocumentsBecomeTemplates)
{
    GamedataTables tables;
    const optional<string> error = GamedataParser::Parse(MakeValidDocuments(), OUT tables);
    ASSERT_FALSE(error.has_value()) << error.value_or("");

    const ItemTemplate& sword = tables.items.at(SWORD_ID);
    EXPECT_EQ(sword.itemType, Protocol::ITEM_TYPE_GEAR);
    ASSERT_TRUE(sword.gearType.has_value());
    EXPECT_EQ(sword.gearType.value(), Protocol::GEAR_TYPE_WEAPON);
    EXPECT_EQ(sword.physicalAttack, 15);
    EXPECT_EQ(sword.cooldownMs, 0u) << "장비의 cooldown -1은 대기가 없다는 뜻이다";
    EXPECT_EQ(sword.levelRequirement, 10);
    ASSERT_TRUE(sword.classRequirement.has_value());
    EXPECT_EQ(sword.classRequirement.value(), Protocol::CLASS_TYPE_WARRIOR);

    const ItemTemplate& potion = tables.items.at(POTION_ID);
    EXPECT_EQ(potion.itemType, Protocol::ITEM_TYPE_CONSUMABLE);
    EXPECT_FALSE(potion.gearType.has_value());
    EXPECT_EQ(potion.maxStack, 99);
    EXPECT_EQ(potion.cooldownMs, 10000u);
    EXPECT_DOUBLE_EQ(potion.hpRestoreRatio, 0.3);
    EXPECT_EQ(potion.hp, 0) << "표에 없는 스탯은 0이다";
    EXPECT_FALSE(potion.classRequirement.has_value()) << "\"all\"은 직업을 가리지 않는다";

    const MonsterTemplate& monster = tables.monsters.at(MONSTER_ID);
    EXPECT_EQ(monster.maxHp, 200);
    EXPECT_EQ(monster.minExp, 5);
    EXPECT_EQ(monster.maxGold, 20);
    EXPECT_FLOAT_EQ(monster.attackInterval, 2.5f);

    const ClassLevelTable& warrior = tables.classLevelTables.at(Protocol::CLASS_TYPE_WARRIOR);
    EXPECT_EQ(warrior.GetMaxLevel(), 3) << "최대 레벨은 레벨 표의 마지막 레벨이다";
    ASSERT_NE(warrior.Find(2), nullptr);
    EXPECT_EQ(warrior.Find(2)->maxHp, 502);
    EXPECT_EQ(warrior.Find(4), nullptr);
    EXPECT_FALSE(tables.classLevelTables.contains(Protocol::CLASS_TYPE_NONE)) << "NONE에는 레벨 표가 없다";

    EXPECT_EQ(tables.townRoomId, TOWN_ID) << "마을은 리스폰 지점이 있는 맵이다";
    const MapTemplate& town = tables.maps.at(TOWN_ID);
    ASSERT_TRUE(town.respawnPoint.has_value());
    EXPECT_FLOAT_EQ(town.respawnPoint->y, 20.f);
    ASSERT_EQ(town.portals.size(), 1u);
    EXPECT_EQ(town.portals[0].portalId, 11);
    EXPECT_EQ(town.portals[0].dstRoomId, FIELD_ID);
    EXPECT_FLOAT_EQ(town.portals[0].dstYaw, 180.f);
    EXPECT_FLOAT_EQ(town.portals[0].srcPos.x, -1200.f);
    EXPECT_FLOAT_EQ(town.portals[0].srcPos.y, 1000.f);
    EXPECT_FLOAT_EQ(town.portalRadius, 500.f);
    EXPECT_EQ(town.mapId, 1111);
    EXPECT_EQ(town.maxMonsterCount, 0) << "null인 몬스터 수는 0이다";

    const MapTemplate& field = tables.maps.at(FIELD_ID);
    EXPECT_FALSE(field.respawnPoint.has_value());
    EXPECT_EQ(field.monsterIds, vector<int32>({MONSTER_ID}));
    EXPECT_EQ(field.maxMonsterCount, 10);
    EXPECT_FLOAT_EQ(field.portalRadius, 300.5f);
}

TEST(GamedataParserTest, MissingFieldNamesFileRowAndField)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.items[1].erase("buyPrice");

    ExpectMentions(ErrorOf(documents), {GamedataFile::ITEMS, "2번째 행", "templateId 2000", "buyPrice"});
}

TEST(GamedataParserTest, WrongFieldTypeIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.monsters[0]["maxHp"] = "200";

    ExpectMentions(ErrorOf(documents), {GamedataFile::MONSTERS, "1번째 행", "maxHp"});
}

TEST(GamedataParserTest, NestedFieldIsChecked)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.monsters[0]["expReward"].erase("maxExp");

    ExpectMentions(ErrorOf(documents), {GamedataFile::MONSTERS, "maxExp"});
}

TEST(GamedataParserTest, UnknownItemTypeIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.items[0]["itemType"] = "WEAPON";

    ExpectMentions(ErrorOf(documents), {GamedataFile::ITEMS, "1번째 행", "itemType", "WEAPON"});
}

// itemType에는 아이템 종류만 온다. 없앤 단계의 값(방어구·무기·소비)이나 종류가 아닌 값을 종류로 읽으면
// 기획 원본이 다시 어긋나도 알아채지 못한다.
TEST(GamedataParserTest, ItemTypeOtherThanKindIsRejected)
{
    for (const char* itemType : { "weapon", "consumption", "NONE", "gear", "" })
    {
        GamedataDocuments documents = MakeValidDocuments();
        documents.items[1]["itemType"] = itemType;

        SCOPED_TRACE(itemType);
        ExpectMentions(ErrorOf(documents), {GamedataFile::ITEMS, "itemType"});
    }
}

TEST(GamedataParserTest, GearWithUnknownSubtypeIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.items[0]["itemSubtype"] = "shield";

    ExpectMentions(ErrorOf(documents), {GamedataFile::ITEMS, "itemSubtype", "shield"});
}

TEST(GamedataParserTest, UnknownClassRequirementIsRejected)
{
    for (const char* classRequirement : { "archer", "none", "" })
    {
        GamedataDocuments documents = MakeValidDocuments();
        documents.items[0]["classRequirement"] = classRequirement;

        SCOPED_TRACE(classRequirement);
        ExpectMentions(ErrorOf(documents), {GamedataFile::ITEMS, "1번째 행", "classRequirement"});
    }
}

TEST(GamedataParserTest, MaxStackBelowOneIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.items[1]["maxStack"] = 0;

    ExpectMentions(ErrorOf(documents), {GamedataFile::ITEMS, "maxStack"});
}

TEST(GamedataParserTest, RewardMinAboveMaxIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.monsters[0]["goldReward"]["minGold"] = 30;

    ExpectMentions(ErrorOf(documents), {GamedataFile::MONSTERS, "minGold"});
}

TEST(GamedataParserTest, DuplicateTemplateIdIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.items.push_back(MakeSword());

    ExpectMentions(ErrorOf(documents), {GamedataFile::ITEMS, "3번째 행", "중복"});
}

TEST(GamedataParserTest, LevelRowsMustStartAtOneWithoutGaps)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.warriorLevels = Json::array({MakeLevelRow(1), MakeLevelRow(3)});

    ExpectMentions(ErrorOf(documents), {GamedataFile::WARRIOR_LEVELS, "2번째 행", "level"});

    documents.warriorLevels = Json::array();
    ExpectMentions(ErrorOf(documents), {GamedataFile::WARRIOR_LEVELS});
}

TEST(GamedataParserTest, RespawnMapWithoutRespawnPointIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.maps[0]["respawnPoint"] = nullptr;

    ExpectMentions(ErrorOf(documents), {GamedataFile::MAPS, "1번째 행", "respawnPoint"});
}

TEST(GamedataParserTest, ExactlyOneTownIsRequired)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.maps[0]["hasRespawnPoint"] = false;
    ExpectMentions(ErrorOf(documents), {GamedataFile::MAPS, "마을"});

    documents = MakeValidDocuments();
    documents.maps[1]["hasRespawnPoint"] = true;
    documents.maps[1]["respawnPoint"] = MakePos(0, 0, 0);
    ExpectMentions(ErrorOf(documents), {GamedataFile::MAPS, "마을"});
}

TEST(GamedataParserTest, PortalToUnknownMapIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.maps[0]["portals"]["lists"][0]["dst"]["templateId"] = 99;

    ExpectMentions(ErrorOf(documents), {GamedataFile::MAPS, "templateId 10", "99"});
}

// 반경이 0 이하면 어느 위치에서도 포털을 탈 수 없다. 표가 틀린 것이므로 부팅에서 막는다.
TEST(GamedataParserTest, NonPositivePortalRadiusIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.maps[1]["portalRadius"] = 0;

    ExpectMentions(ErrorOf(documents), {GamedataFile::MAPS, "2번째 행", "portalRadius"});
}

TEST(GamedataParserTest, UnknownMonsterInMapIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.maps[1]["monsterIds"] = Json::array({MONSTER_ID, 5999});

    ExpectMentions(ErrorOf(documents), {GamedataFile::MAPS, "templateId 20", "5999"});
}

TEST(GamedataParserTest, FailedLoadKeepsPreviousTables)
{
    ASSERT_FALSE(Gamedata::Load(MakeValidDocuments()).has_value());
    ASSERT_NE(Gamedata::FindItem(SWORD_ID), nullptr);

    GamedataDocuments broken = MakeValidDocuments();
    broken.items[0].erase("itemType");
    broken.items[1]["templateId"] = 2999;

    EXPECT_TRUE(Gamedata::Load(broken).has_value());
    EXPECT_NE(Gamedata::FindItem(SWORD_ID), nullptr) << "실패한 불러오기가 이전 표를 지웠다";
    EXPECT_EQ(Gamedata::FindItem(2999), nullptr) << "실패한 불러오기가 일부 행을 설치했다";

    Gamedata::Install(GamedataTables());
}

TEST(GamedataParserTest, RepositoryGamedataPasses)
{
    // 테스트 소스 위치에서 GameServer의 기획표 폴더를 찾는다. 작업 디렉터리와 무관하게 돈다.
    const filesystem::path dir = filesystem::path(__FILE__).parent_path() / ".." / "GameServer" / "Game" / "Data" / "Json";

    GamedataDocuments documents;
    documents.warriorLevels = ReadJsonFile(dir / GamedataFile::WARRIOR_LEVELS);
    documents.items = ReadJsonFile(dir / GamedataFile::ITEMS);
    documents.maps = ReadJsonFile(dir / GamedataFile::MAPS);
    documents.monsters = ReadJsonFile(dir / GamedataFile::MONSTERS);
    ASSERT_TRUE(documents.items.is_array()) << "기획표를 찾지 못했다: " << dir.string();

    GamedataTables tables;
    const optional<string> error = GamedataParser::Parse(documents, OUT tables);
    EXPECT_FALSE(error.has_value()) << error.value_or("");
    EXPECT_EQ(tables.townRoomId, 10);
    EXPECT_EQ(tables.classLevelTables.at(Protocol::CLASS_TYPE_WARRIOR).GetMaxLevel(), 50);
}
