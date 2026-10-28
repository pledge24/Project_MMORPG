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
    constexpr int32 DROP_GROUP_ID = 1;
    constexpr int32 MAP_ID = 1111;
    constexpr int32 TOWN_ID = 10;
    constexpr int32 FIELD_ID = 20;
    constexpr int32 GOLD_ID = 1;
    constexpr const char* PLACEMENT_FILE = "Placement_1111.json";

    /** DataBuilder가 생성하는 레벨 표의 행. */
    Json MakeLevel(int32 level, int64 expRequirement)
    {
        return Json{{"Level", level}, {"ExpRequirement", expRequirement}};
    }

    /** DataBuilder가 생성하는 직업별 능력치 표의 행. */
    Json MakeWarriorLevelStat(int32 level)
    {
        return Json{
            {"Class", "WARRIOR"}, {"Level", level}, {"MaxHp", 500 + level}, {"MaxMp", 100}, {"PhysicalAttack", 10}, {"MagicalAttack", 2},
        };
    }

    /** DataBuilder가 생성하는 아이템 표의 행. */
    Json MakeSword()
    {
        return Json{
            {"Id", SWORD_ID}, {"Type", "GEAR"}, {"LevelRequirement", 10}, {"AllowedClasses", Json::array({"WARRIOR"})},
            {"BuyPrice", 100}, {"SellPrice", 10}, {"Sellable", true}, {"MaxStack", 1},
        };
    }

    Json MakePotion()
    {
        return Json{
            {"Id", POTION_ID}, {"Type", "CONSUMABLE"}, {"LevelRequirement", 0}, {"AllowedClasses", Json::array()},
            {"BuyPrice", 50}, {"SellPrice", 5}, {"Sellable", true}, {"MaxStack", 99},
        };
    }

    Json MakeSwordEquipStat()
    {
        return Json{{"ItemId", SWORD_ID}, {"Part", "WEAPON"}, {"Hp", 0}, {"Mp", 0}, {"PhysicalAttack", 15}, {"MagicalAttack", 0}};
    }

    Json MakePotionConsumeEffect()
    {
        return Json{{"ItemId", POTION_ID}, {"HpRestoreRatio", 0.3}, {"MpRestoreRatio", 0.0}, {"CooldownMs", 10000}};
    }

    /** DataBuilder가 생성하는 몬스터 표의 행. */
    Json MakeMonster()
    {
        return Json{
            {"Id", MONSTER_ID}, {"MaxHp", 200}, {"BaseAttack", 100}, {"MovementSpeed", 300.0},
            {"MinExp", 5}, {"MaxExp", 10}, {"MinGold", 10}, {"MaxGold", 20},
            {"AttackInterval", 2.5}, {"TryAttackRange", 150.0}, {"DetectionRange", 900.0}, {"ChasingMaxRange", 1200.0},
            {"DropGroups", Json::array({DROP_GROUP_ID})},
        };
    }

    /** DataBuilder가 생성하는 드롭 그룹 표의 행. 드롭 항목은 그룹 아래 Entries에 중첩된다. */
    Json MakeDropGroup()
    {
        return Json{
            {"Id", DROP_GROUP_ID}, {"Method", "WEIGHTED"}, {"Rate", 0.5}, {"PickCount", 1},
            {"Entries", Json::array({
                Json{{"ItemId", SWORD_ID}, {"Rate", nullptr}, {"Weight", 10}, {"MinCount", 1}, {"MaxCount", 1}},
                Json{{"ItemId", POTION_ID}, {"Rate", nullptr}, {"Weight", 30}, {"MinCount", 2}, {"MaxCount", 5}},
            })},
        };
    }

    /** DataBuilder가 생성하는 맵 표의 행. 이름은 클라이언트만 쓴다. */
    Json MakeMap()
    {
        return Json{{"Id", MAP_ID}};
    }

    /** DataBuilder가 생성하는 지역 표의 행. 포털은 지역 아래 Portals에 중첩된다. */
    Json MakeTown()
    {
        Json portal = {
            {"Id", 11}, {"Type", "AREA"}, {"SrcX", -1200}, {"SrcY", 1000}, {"SrcZ", 0}, {"Radius", 500},
            {"DestAreaId", FIELD_ID}, {"DestX", -8000}, {"DestY", 10000}, {"DestZ", 0}, {"DestYaw", 180.0},
        };

        return Json{
            {"Id", TOWN_ID}, {"MapId", MAP_ID}, {"HasRespawnPoint", true},
            {"Monsters", Json::array()}, {"MaxMonsterCount", 0}, {"MonsterRespawnSec", 0},
            {"CenterX", 0}, {"CenterY", 0}, {"CenterZ", 0}, {"HalfExtentX", 1500}, {"HalfExtentY", 1500},
            {"RespawnX", 10}, {"RespawnY", 20}, {"RespawnZ", 0},
            {"Portals", Json::array({portal})},
        };
    }

    Json MakeField()
    {
        return Json{
            {"Id", FIELD_ID}, {"MapId", MAP_ID}, {"HasRespawnPoint", false},
            {"Monsters", Json::array({MONSTER_ID})}, {"MaxMonsterCount", 10}, {"MonsterRespawnSec", 10},
            {"CenterX", -10000}, {"CenterY", 10000}, {"CenterZ", 0}, {"HalfExtentX", 2000}, {"HalfExtentY", 1000.5},
            {"RespawnX", nullptr}, {"RespawnY", nullptr}, {"RespawnZ", nullptr},
            {"Portals", Json::array()},
        };
    }

    /** DataBuilder가 생성하는 재화 표의 한 행. */
    Json MakeCurrency(int32 id, const char* type)
    {
        return Json{{"Id", id}, {"Name", "골드"}, {"Type", type}};
    }

    /** DataBuilder가 생성하는 초기 진행 설정. 표가 아니라 객체 하나다. */
    Json MakeInitialProgress()
    {
        return Json{
            {"StartMapId", MAP_ID}, {"StartX", 100.0}, {"StartY", -200.0}, {"StartZ", 0.0}, {"StartYaw", 90.0}, {"StartGold", 10000},
        };
    }

    /** 추출 커맨드렛이 쓰는 맵의 배치 데이터. 마을과 필드의 볼륨, 마을의 리스폰 지점, 마을에서 필드로 가는 포털이 있다. */
    Json MakePlacement()
    {
        return Json{
            {"MapId", MAP_ID}, {"CenterX", -5000}, {"CenterY", 5000}, {"HalfExtentX", 8000}, {"HalfExtentY", 8000}, {"CellSize", 1000},
            {"Areas", Json::array({
                Json{{"Id", TOWN_ID}, {"CenterX", 0}, {"CenterY", 0}, {"CenterZ", 0}, {"HalfExtentX", 1500}, {"HalfExtentY", 1500}},
                Json{{"Id", FIELD_ID}, {"CenterX", -10000}, {"CenterY", 10000}, {"CenterZ", 50}, {"HalfExtentX", 2000}, {"HalfExtentY", 1000.5}},
            })},
            {"RespawnPoints", Json::array({
                Json{{"AreaId", TOWN_ID}, {"X", 10}, {"Y", 20}, {"Z", 0}, {"Yaw", 90}},
            })},
            {"Portals", Json::array({
                Json{
                    {"Id", 11}, {"SrcX", -1200}, {"SrcY", 1000}, {"SrcZ", 0}, {"Radius", 500},
                    {"DestX", -8000}, {"DestY", 10000}, {"DestZ", 0}, {"DestYaw", 180},
                },
            })},
        };
    }

    GamedataDocuments MakeValidDocuments()
    {
        GamedataDocuments documents;
        documents.levels = Json::array({MakeLevel(1, 50), MakeLevel(2, 100), MakeLevel(3, 0)});
        documents.classLevelStats = Json::array({MakeWarriorLevelStat(1), MakeWarriorLevelStat(2), MakeWarriorLevelStat(3)});
        documents.items = Json::array({MakeSword(), MakePotion()});
        documents.itemEquipStats = Json::array({MakeSwordEquipStat()});
        documents.itemConsumeEffects = Json::array({MakePotionConsumeEffect()});
        documents.monsters = Json::array({MakeMonster()});
        documents.dropGroups = Json::array({MakeDropGroup()});
        documents.maps = Json::array({MakeMap()});
        documents.areas = Json::array({MakeTown(), MakeField()});
        documents.currencies = Json::array({MakeCurrency(GOLD_ID, "GOLD")});
        documents.initialProgress = MakeInitialProgress();
        documents.placements[PLACEMENT_FILE] = MakePlacement();
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

    const DesignData::ItemRow& sword = tables.items.at(SWORD_ID);
    EXPECT_EQ(sword.Type, Protocol::ITEM_TYPE_GEAR);
    EXPECT_EQ(sword.LevelRequirement, 10);
    EXPECT_EQ(sword.AllowedClasses, vector<Protocol::CharacterClass>({Protocol::CHARACTER_CLASS_WARRIOR}));
    const DesignData::ItemEquipStatRow& swordStat = tables.itemEquipStats.at(SWORD_ID);
    EXPECT_EQ(swordStat.Part, Protocol::GEAR_PART_WEAPON);
    EXPECT_EQ(swordStat.PhysicalAttack, 15);
    EXPECT_FALSE(tables.itemConsumeEffects.contains(SWORD_ID));

    const DesignData::ItemRow& potion = tables.items.at(POTION_ID);
    EXPECT_EQ(potion.Type, Protocol::ITEM_TYPE_CONSUMABLE);
    EXPECT_EQ(potion.MaxStack, 99);
    EXPECT_TRUE(potion.AllowedClasses.empty()) << "빈 직업 목록은 직업을 가리지 않는다";
    const DesignData::ItemConsumeEffectRow& potionEffect = tables.itemConsumeEffects.at(POTION_ID);
    EXPECT_EQ(potionEffect.CooldownMs, 10000);
    EXPECT_FLOAT_EQ(potionEffect.HpRestoreRatio, 0.3f);
    EXPECT_FALSE(tables.itemEquipStats.contains(POTION_ID));

    const DesignData::MonsterRow& monster = tables.monsters.at(MONSTER_ID);
    EXPECT_EQ(monster.MaxHp, 200);
    EXPECT_EQ(monster.MinExp, 5);
    EXPECT_EQ(monster.MaxGold, 20);
    EXPECT_FLOAT_EQ(monster.AttackInterval, 2.5f);
    EXPECT_EQ(monster.DropGroups, vector<int32>({DROP_GROUP_ID}));

    ASSERT_EQ(tables.levels.size(), 3u);
    EXPECT_EQ(tables.levels.at(2).ExpRequirement, 100);
    const auto warriorLevel2 = tables.classLevelStats.find({ Protocol::CHARACTER_CLASS_WARRIOR, 2 });
    ASSERT_NE(warriorLevel2, tables.classLevelStats.end());
    EXPECT_EQ(warriorLevel2->second.MaxHp, 502);
    EXPECT_EQ(warriorLevel2->second.MagicalAttack, 2);
    EXPECT_FALSE(tables.classLevelStats.contains({ Protocol::CHARACTER_CLASS_MAGE, 1 })) << "표에 없는 직업에는 행이 없다";

    ASSERT_TRUE(tables.maps.contains(MAP_ID));
    EXPECT_EQ(tables.townAreaId, TOWN_ID) << "마을은 리스폰 지점이 있는 지역이다";
    const DesignData::AreaRow& town = tables.areas.at(TOWN_ID);
    EXPECT_EQ(town.MapId, MAP_ID);
    EXPECT_EQ(town.RespawnY, optional<float>(20.f));
    ASSERT_EQ(town.Portals.size(), 1u);
    EXPECT_EQ(town.Portals[0].Id, 11);
    EXPECT_EQ(town.Portals[0].DestAreaId, FIELD_ID);
    EXPECT_FLOAT_EQ(town.Portals[0].DestYaw, 180.f);
    EXPECT_FLOAT_EQ(town.Portals[0].SrcX, -1200.f);
    EXPECT_FLOAT_EQ(town.Portals[0].SrcY, 1000.f);
    EXPECT_FLOAT_EQ(town.Portals[0].Radius, 500.f);
    EXPECT_EQ(town.MaxMonsterCount, 0);

    const DesignData::AreaRow& field = tables.areas.at(FIELD_ID);
    EXPECT_EQ(field.RespawnX, nullopt);
    EXPECT_EQ(field.Monsters, vector<int32>({MONSTER_ID}));
    EXPECT_EQ(field.MaxMonsterCount, 10);
    EXPECT_FLOAT_EQ(field.HalfExtentY, 1000.5f);
    EXPECT_TRUE(field.Portals.empty());

    EXPECT_EQ(tables.goldCurrencyId, GOLD_ID) << "골드는 재화 종류가 GOLD인 재화다";
    ASSERT_TRUE(tables.currencies.contains(GOLD_ID));
    EXPECT_EQ(tables.currencies.at(GOLD_ID).Type, DesignData::CurrencyType::GOLD);

    const DesignData::ProgressConfig& initial = tables.initialProgress;
    EXPECT_EQ(initial.StartMapId, MAP_ID);
    EXPECT_FLOAT_EQ(initial.StartX, 100.f);
    EXPECT_FLOAT_EQ(initial.StartY, -200.f);
    EXPECT_FLOAT_EQ(initial.StartYaw, 90.f);
    EXPECT_EQ(initial.StartGold, 10000);
}

TEST(GamedataParserTest, PlacementBecomesMapAreaPortalAndRespawnSpace)
{
    GamedataTables tables;
    const optional<string> error = GamedataParser::Parse(MakeValidDocuments(), OUT tables);
    ASSERT_FALSE(error.has_value()) << error.value_or("");

    const MapPlacement& map = tables.mapPlacements.at(MAP_ID);
    EXPECT_FLOAT_EQ(map.cellSize, 1000.f);
    EXPECT_FLOAT_EQ(map.bounds.minX, -13000.f);
    EXPECT_FLOAT_EQ(map.bounds.maxY, 13000.f);

    const AreaPlacement& field = tables.areaPlacements.at(FIELD_ID);
    EXPECT_FLOAT_EQ(field.center.z, 50.f);
    EXPECT_FLOAT_EQ(field.bounds.minX, -12000.f);
    EXPECT_FLOAT_EQ(field.bounds.maxX, -8000.f);
    EXPECT_FLOAT_EQ(field.bounds.minY, 8999.5f);
    EXPECT_FLOAT_EQ(field.bounds.maxY, 11000.5f);
    EXPECT_TRUE(tables.areaPlacements.contains(TOWN_ID));

    const RespawnPointPlacement& respawn = tables.respawnPoints.at(TOWN_ID);
    EXPECT_FLOAT_EQ(respawn.pos.y, 20.f);
    EXPECT_FLOAT_EQ(respawn.yaw, 90.f);

    const PortalPlacement& portal = tables.portalPlacements.at(11);
    EXPECT_FLOAT_EQ(portal.src.x, -1200.f);
    EXPECT_FLOAT_EQ(portal.radius, 500.f);
    EXPECT_FLOAT_EQ(portal.dest.y, 10000.f);
    EXPECT_FLOAT_EQ(portal.destYaw, 180.f);
}

TEST(GamedataParserTest, MissingPlacementFieldNamesFileAndField)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.placements[PLACEMENT_FILE]["Portals"][0].erase("Radius");

    ExpectMentions(ErrorOf(documents), {PLACEMENT_FILE, "'Portals' 1번째 원소", "Radius"});
}

TEST(GamedataParserTest, MapWithoutPlacementIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.placements.clear();

    ExpectMentions(ErrorOf(documents), {"맵 1111", "배치 데이터가 없다"});
}

TEST(GamedataParserTest, PlacementOfUnknownMapIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.placements["Placement_9999.json"] = MakePlacement();
    documents.placements["Placement_9999.json"]["MapId"] = 9999;
    documents.placements["Placement_9999.json"]["Areas"] = Json::array();
    documents.placements["Placement_9999.json"]["RespawnPoints"] = Json::array();
    documents.placements["Placement_9999.json"]["Portals"] = Json::array();

    ExpectMentions(ErrorOf(documents), {"Placement_9999.json", "맵 9999", "맵 표에 없다"});
}

TEST(GamedataParserTest, PlacedAreaNotInAreaTableIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.placements[PLACEMENT_FILE]["Areas"].push_back(
        Json{{"Id", 99}, {"CenterX", 0}, {"CenterY", 0}, {"CenterZ", 0}, {"HalfExtentX", 1}, {"HalfExtentY", 1}});

    ExpectMentions(ErrorOf(documents), {PLACEMENT_FILE, "지역 99", "지역 표에 없다"});
}

TEST(GamedataParserTest, AreaPlacedInOtherMapIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.maps.push_back(Json{{"Id", 2222}});
    documents.areas[1]["MapId"] = 2222;

    ExpectMentions(ErrorOf(documents), {PLACEMENT_FILE, "지역 20", "맵 2222"});
}

TEST(GamedataParserTest, AreaWithoutPlacementIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.placements[PLACEMENT_FILE]["Areas"].erase(1);

    ExpectMentions(ErrorOf(documents), {"지역 20", "배치 데이터가 없다"});
}

TEST(GamedataParserTest, PlacedPortalNotInPortalTableIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    Json unknown = documents.placements[PLACEMENT_FILE]["Portals"][0];
    unknown["Id"] = 99;
    documents.placements[PLACEMENT_FILE]["Portals"].push_back(unknown);

    ExpectMentions(ErrorOf(documents), {PLACEMENT_FILE, "포털 99", "포털 표에 없다"});
}

TEST(GamedataParserTest, PortalWithoutPlacementIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.placements[PLACEMENT_FILE]["Portals"] = Json::array();

    ExpectMentions(ErrorOf(documents), {"포털 11", "배치 데이터가 없다"});
}

TEST(GamedataParserTest, TownWithoutRespawnPointIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.placements[PLACEMENT_FILE]["RespawnPoints"] = Json::array();

    ExpectMentions(ErrorOf(documents), {"지역 10", "리스폰 지점"});
}

TEST(GamedataParserTest, RespawnPointInAreaWithoutRespawnIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    Json fieldRespawn = documents.placements[PLACEMENT_FILE]["RespawnPoints"][0];
    fieldRespawn["AreaId"] = FIELD_ID;
    documents.placements[PLACEMENT_FILE]["RespawnPoints"].push_back(fieldRespawn);

    ExpectMentions(ErrorOf(documents), {PLACEMENT_FILE, "지역 20", "리스폰 지점이 없는 지역"});
}

TEST(GamedataParserTest, MissingFieldNamesFileRowAndField)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.items[1].erase("BuyPrice");

    ExpectMentions(ErrorOf(documents), {DesignData::ITEM_FILE, "2번째 행", "BuyPrice"});
}

TEST(GamedataParserTest, WrongFieldTypeIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.monsters[0]["MaxHp"] = "200";

    ExpectMentions(ErrorOf(documents), {DesignData::MONSTER_FILE, "1번째 행", "MaxHp"});
}

TEST(GamedataParserTest, NestedDropEntryFieldIsChecked)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.dropGroups[0]["Entries"][1].erase("Weight");

    ExpectMentions(ErrorOf(documents), {DesignData::DROP_GROUP_FILE, "1번째 행", "'Entries' 2번째 원소", "Weight"});
}

TEST(GamedataParserTest, UnknownDropMethodIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.dropGroups[0]["Method"] = "RANDOM";

    ExpectMentions(ErrorOf(documents), {DesignData::DROP_GROUP_FILE, "1번째 행", "Method", "RANDOM"});
}

// 아이템 표의 규칙(종류별 표의 짝, 번호 범위, 최대 개수)은 DataBuilder가 검사한다.
// 서버는 생성된 역직렬화로 형식만 본다. 열거형은 셀에 적는 값 이름만 읽는다.
TEST(GamedataParserTest, ItemTypeOtherThanValueNameIsRejected)
{
    for (const char* itemType : { "WEAPON", "ITEM_TYPE_GEAR", "NONE", "gear", "" })
    {
        GamedataDocuments documents = MakeValidDocuments();
        documents.items[1]["Type"] = itemType;

        SCOPED_TRACE(itemType);
        ExpectMentions(ErrorOf(documents), {DesignData::ITEM_FILE, "2번째 행", "Type"});
    }
}

TEST(GamedataParserTest, UnknownGearPartIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.itemEquipStats[0]["Part"] = "SHIELD";

    ExpectMentions(ErrorOf(documents), {DesignData::ITEM_EQUIP_STAT_FILE, "1번째 행", "Part", "SHIELD"});
}

TEST(GamedataParserTest, UnknownAllowedClassIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.items[0]["AllowedClasses"] = Json::array({"WARRIOR", "ARCHER"});

    ExpectMentions(ErrorOf(documents), {DesignData::ITEM_FILE, "1번째 행", "AllowedClasses", "ARCHER"});
}

TEST(GamedataParserTest, MissingConsumeEffectFieldIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.itemConsumeEffects[0].erase("CooldownMs");

    ExpectMentions(ErrorOf(documents), {DesignData::ITEM_CONSUME_EFFECT_FILE, "1번째 행", "CooldownMs"});
}

TEST(GamedataParserTest, DuplicateTemplateIdIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.items.push_back(MakeSword());

    ExpectMentions(ErrorOf(documents), {DesignData::ITEM_FILE, "3번째 행", "키가 겹친다"});
}

TEST(GamedataParserTest, ClassLevelStatWithUnknownClassIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.classLevelStats[1]["Class"] = "PALADIN";

    ExpectMentions(ErrorOf(documents), {DesignData::CLASS_LEVEL_STAT_FILE, "2번째 행", "Class"});
}

// 마을이 없으면 사망한 플레이어를 보낼 곳이 없다. 마을이 둘 이상인지는 DataBuilder가 검사한다.
TEST(GamedataParserTest, MissingTownIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.areas[0]["HasRespawnPoint"] = false;

    ExpectMentions(ErrorOf(documents), {DesignData::AREA_FILE, "마을"});
}

TEST(GamedataParserTest, NestedPortalFieldIsChecked)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.areas[0]["Portals"][0].erase("Radius");

    ExpectMentions(ErrorOf(documents), {DesignData::AREA_FILE, "1번째 행", "'Portals' 1번째 원소", "Radius"});
}

// 서버는 재화 종류로 골드를 찾는다. 골드가 없으면 DB에 쓸 재화 번호가 없다.
// GOLD인 행이 둘 이상인지는 DataBuilder가 검사한다.
TEST(GamedataParserTest, MissingGoldCurrencyIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.currencies = Json::array();

    ExpectMentions(ErrorOf(documents), {DesignData::CURRENCY_FILE, "GOLD"});
}

// 생성된 역직렬화가 형식을 확인한다. 생성된 열거형에 없는 값은 읽지 않는다.
TEST(GamedataParserTest, UnknownCurrencyTypeIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.currencies.push_back(MakeCurrency(2, "HONOR"));

    ExpectMentions(ErrorOf(documents), {DesignData::CURRENCY_FILE, "2번째 행", "Type", "HONOR"});
}

TEST(GamedataParserTest, DuplicateCurrencyIdIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.currencies.push_back(MakeCurrency(GOLD_ID, "GOLD"));

    ExpectMentions(ErrorOf(documents), {DesignData::CURRENCY_FILE, "2번째 행", "키가 겹친다"});
}

// 시작 맵이 맵 표에 있는지와 시작 골드가 0 이상인지는 DataBuilder가 검사한다. 서버는 설정의 형식만 본다.
TEST(GamedataParserTest, MalformedInitialProgressIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.initialProgress.erase("StartGold");
    ExpectMentions(ErrorOf(documents), {DesignData::PROGRESS_FILE, "StartGold"});

    documents = MakeValidDocuments();
    documents.initialProgress = Json::array({MakeInitialProgress()});
    ExpectMentions(ErrorOf(documents), {DesignData::PROGRESS_FILE});
}

// 드롭의 판정과 지급은 아직 없다. 서버는 드롭 그룹을 생성된 타입으로 읽어 설치만 한다.
TEST(GamedataParserTest, DropGroupsAreInstalledAsGeneratedRows)
{
    ASSERT_FALSE(Gamedata::Load(MakeValidDocuments()).has_value());

    const DesignData::DropGroupRow* group = Gamedata::FindDropGroup(DROP_GROUP_ID);
    ASSERT_NE(group, nullptr);
    EXPECT_EQ(group->Method, DesignData::DropMethod::WEIGHTED);
    EXPECT_FLOAT_EQ(group->Rate, 0.5f);
    EXPECT_EQ(group->PickCount, optional<int32>(1));
    ASSERT_EQ(group->Entries.size(), 2u);
    EXPECT_EQ(group->Entries[1].ItemId, POTION_ID);
    EXPECT_EQ(group->Entries[1].Rate, nullopt) << "가중치 그룹의 항목은 확률이 비어 있다";
    EXPECT_EQ(group->Entries[1].Weight, optional<int32>(30));
    EXPECT_EQ(group->Entries[1].MinCount, 2);
    EXPECT_EQ(group->Entries[1].MaxCount, 5);
    EXPECT_EQ(Gamedata::FindDropGroup(DROP_GROUP_ID + 1), nullptr);

    Gamedata::Install(GamedataTables());
}

TEST(GamedataParserTest, FailedLoadKeepsPreviousTables)
{
    ASSERT_FALSE(Gamedata::Load(MakeValidDocuments()).has_value());
    ASSERT_NE(Gamedata::FindItem(SWORD_ID), nullptr);

    GamedataDocuments broken = MakeValidDocuments();
    broken.items[0].erase("Type");
    broken.items[1]["Id"] = 2999;

    EXPECT_TRUE(Gamedata::Load(broken).has_value());
    EXPECT_NE(Gamedata::FindItem(SWORD_ID), nullptr) << "실패한 불러오기가 이전 표를 지웠다";
    EXPECT_EQ(Gamedata::FindItem(2999), nullptr) << "실패한 불러오기가 일부 행을 설치했다";

    Gamedata::Install(GamedataTables());
}

TEST(GamedataParserTest, RepositoryGamedataPasses)
{
    // 테스트 소스 위치에서 GameServer의 기획표 폴더를 찾는다. 작업 디렉터리와 무관하게 돈다.
    const filesystem::path generatedDir = filesystem::path(__FILE__).parent_path() / ".." / "GameServer" / "Game" / "Data" / "Generated";

    GamedataDocuments documents;
    documents.levels = ReadJsonFile(generatedDir / DesignData::LEVEL_FILE);
    documents.classLevelStats = ReadJsonFile(generatedDir / DesignData::CLASS_LEVEL_STAT_FILE);
    documents.items = ReadJsonFile(generatedDir / DesignData::ITEM_FILE);
    documents.itemEquipStats = ReadJsonFile(generatedDir / DesignData::ITEM_EQUIP_STAT_FILE);
    documents.itemConsumeEffects = ReadJsonFile(generatedDir / DesignData::ITEM_CONSUME_EFFECT_FILE);
    documents.maps = ReadJsonFile(generatedDir / DesignData::MAP_FILE);
    documents.areas = ReadJsonFile(generatedDir / DesignData::AREA_FILE);
    documents.monsters = ReadJsonFile(generatedDir / DesignData::MONSTER_FILE);
    documents.dropGroups = ReadJsonFile(generatedDir / DesignData::DROP_GROUP_FILE);
    documents.currencies = ReadJsonFile(generatedDir / DesignData::CURRENCY_FILE);
    documents.initialProgress = ReadJsonFile(generatedDir / DesignData::PROGRESS_FILE);
    ASSERT_TRUE(documents.items.is_array()) << "생성된 기획 데이터를 찾지 못했다: " << generatedDir.string();

    const filesystem::path extractedDir = generatedDir / ".." / "Extracted";
    for (const filesystem::directory_entry& entry : filesystem::directory_iterator(extractedDir))
    {
        const string fileName = entry.path().filename().string();
        if (fileName.starts_with("Placement_"))
            documents.placements[fileName] = ReadJsonFile(entry.path());
    }

    GamedataTables tables;
    const optional<string> error = GamedataParser::Parse(documents, OUT tables);
    EXPECT_FALSE(error.has_value()) << error.value_or("");
    EXPECT_EQ(tables.townAreaId, 10);
    ASSERT_FALSE(tables.levels.empty());
    EXPECT_EQ(tables.levels.rbegin()->first, 50);

    // 옛 초기 진행 표에서 설정 워크북으로 옮긴 값이다. 옮기면서 값이 바뀌지 않았는지 본다.
    EXPECT_EQ(tables.initialProgress.StartMapId, 1111);
    EXPECT_FLOAT_EQ(tables.initialProgress.StartX, 0.f);
    EXPECT_FLOAT_EQ(tables.initialProgress.StartYaw, 0.f);
    EXPECT_EQ(tables.initialProgress.StartGold, 10000);
    EXPECT_EQ(tables.goldCurrencyId, 1) << "DB의 CharacterCurrency 행이 골드를 재화 번호 1로 가리킨다";

    // 아이템 표를 공통, 장비 능력치, 소모 효과 표로 나누면서 옮긴 값이다. DB의 아이템 행이 이 번호를 가리킨다.
    ASSERT_TRUE(tables.itemEquipStats.contains(1005));
    EXPECT_EQ(tables.itemEquipStats.at(1005).Part, Protocol::GEAR_PART_WEAPON);
    EXPECT_EQ(tables.itemEquipStats.at(1005).PhysicalAttack, 10);
    ASSERT_TRUE(tables.itemConsumeEffects.contains(2000));
    EXPECT_EQ(tables.itemConsumeEffects.at(2000).CooldownMs, 10000);

    // 전사 레벨 표를 레벨 표와 직업별 능력치 표로 나누면서 옮긴 값이다.
    EXPECT_EQ(tables.levels.at(1).ExpRequirement, 50);
    EXPECT_EQ(tables.levels.at(50).ExpRequirement, 0);
    const DesignData::ClassLevelStatRow& warriorLevel1 = tables.classLevelStats.at({ Protocol::CHARACTER_CLASS_WARRIOR, 1 });
    EXPECT_EQ(warriorLevel1.MaxHp, 500);
    EXPECT_EQ(warriorLevel1.MaxMp, 100);
    EXPECT_EQ(warriorLevel1.PhysicalAttack, 10);
    EXPECT_EQ(warriorLevel1.MagicalAttack, 2);
    EXPECT_EQ(tables.classLevelStats.at({ Protocol::CHARACTER_CLASS_WARRIOR, 50 }).MaxHp, 5400);

    // 몬스터 표를 기본 보상 열로 옮기면서 옮긴 값이다.
    ASSERT_TRUE(tables.monsters.contains(5000));
    EXPECT_EQ(tables.monsters.at(5000).MinGold, 10);
    EXPECT_EQ(tables.monsters.at(5000).MaxGold, 20);
    EXPECT_EQ(tables.monsters.size(), 9u);

    // 맵 표를 맵, 지역, 포털 표로 나누면서 옮긴 값이다. 포털 번호는 클라이언트 레벨의 포털이 보낸다.
    ASSERT_EQ(tables.maps.size(), 1u);
    EXPECT_TRUE(tables.maps.contains(1111));
    ASSERT_EQ(tables.areas.size(), 4u);
    const DesignData::AreaRow& town = tables.areas.at(10);
    EXPECT_EQ(town.RespawnX, optional<float>(0.f));
    ASSERT_EQ(town.Portals.size(), 3u);
    EXPECT_EQ(town.Portals[0].Id, 11);
    EXPECT_EQ(town.Portals[0].DestAreaId, 20);
    EXPECT_FLOAT_EQ(town.Portals[0].Radius, 500.f);
    const DesignData::AreaRow& advancedField = tables.areas.at(40);
    EXPECT_EQ(advancedField.MapId, 1111);
    EXPECT_FLOAT_EQ(advancedField.CenterY, -10000.f);
    EXPECT_FLOAT_EQ(advancedField.HalfExtentX, 2500.f);
    EXPECT_EQ(advancedField.Monsters, vector<int32>({5006, 5007, 5008}));
    ASSERT_EQ(advancedField.Portals.size(), 1u);
    EXPECT_FLOAT_EQ(advancedField.Portals[0].DestY, -1000.f);
}
