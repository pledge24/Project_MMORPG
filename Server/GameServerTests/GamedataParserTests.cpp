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
    constexpr int32 TOWN_ID = 10;
    constexpr int32 FIELD_ID = 20;
    constexpr int32 GOLD_ID = 1;

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

    /** DataBuilder가 생성하는 재화 표의 한 행. */
    Json MakeCurrency(int32 id, const char* type)
    {
        return Json{{"Id", id}, {"Name", "골드"}, {"Type", type}};
    }

    Json MakeInitialProgress()
    {
        return Json{
            {"startMapId", 1111}, {"startPos", MakePos(100, -200, 0)}, {"startYaw", 90.0}, {"startGold", 10000},
        };
    }

    Json MakeInventorySection(const char* itemType, int32 slotCount)
    {
        return Json{{"itemType", itemType}, {"slotCount", slotCount}};
    }

    Json MakeQuickSlot(int32 slotCount)
    {
        return Json{{"slotCount", slotCount}};
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
        documents.maps = Json::array({MakeTown(), MakeField()});
        documents.currencies = Json::array({MakeCurrency(GOLD_ID, "GOLD")});
        documents.initialProgress = Json::array({MakeInitialProgress()});
        documents.inventorySections = Json::array({
            MakeInventorySection("GEAR", 32), MakeInventorySection("CONSUMABLE", 24), MakeInventorySection("MISCELLANEOUS", 16),
        });
        documents.quickSlots = Json::array({MakeQuickSlot(6)});
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

    EXPECT_EQ(tables.goldCurrencyId, GOLD_ID) << "골드는 재화 종류가 GOLD인 재화다";
    ASSERT_TRUE(tables.currencies.contains(GOLD_ID));
    EXPECT_EQ(tables.currencies.at(GOLD_ID).Type, DesignData::CurrencyType::GOLD);

    const InitialProgressTemplate& initial = tables.initialProgress;
    EXPECT_EQ(initial.startMapId, 1111);
    EXPECT_FLOAT_EQ(initial.startPos.x, 100.f);
    EXPECT_FLOAT_EQ(initial.startPos.y, -200.f);
    EXPECT_FLOAT_EQ(initial.startYaw, 90.f);
    EXPECT_EQ(initial.startGold, 10000);

    EXPECT_EQ(tables.inventorySlotCounts.at(Protocol::ITEM_TYPE_GEAR), 32);
    EXPECT_EQ(tables.inventorySlotCounts.at(Protocol::ITEM_TYPE_CONSUMABLE), 24);
    EXPECT_EQ(tables.inventorySlotCounts.at(Protocol::ITEM_TYPE_MISCELLANEOUS), 16);

    EXPECT_EQ(tables.quickSlotCount, 6);
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

// 시작 맵은 맵 표의 mapId다. 룸 번호(templateId)를 적으면 새 캐릭터가 들어갈 맵이 없다.
TEST(GamedataParserTest, StartMapNotInMapTableIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.initialProgress[0]["startMapId"] = TOWN_ID;

    ExpectMentions(ErrorOf(documents), {GamedataFile::INITIAL_PROGRESS, "startMapId", "10"});
}

TEST(GamedataParserTest, InitialProgressMustHaveExactlyOneRow)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.initialProgress = Json::array();
    ExpectMentions(ErrorOf(documents), {GamedataFile::INITIAL_PROGRESS});

    documents = MakeValidDocuments();
    documents.initialProgress.push_back(MakeInitialProgress());
    ExpectMentions(ErrorOf(documents), {GamedataFile::INITIAL_PROGRESS});
}

TEST(GamedataParserTest, NegativeStartGoldIsRejected)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.initialProgress[0]["startGold"] = -1;
    ExpectMentions(ErrorOf(documents), {GamedataFile::INITIAL_PROGRESS, "startGold"});
}

TEST(GamedataParserTest, NonPositiveSectionSlotCountIsRejected)
{
    for (int32 slotCount : { 0, -1 })
    {
        GamedataDocuments documents = MakeValidDocuments();
        documents.inventorySections[1]["slotCount"] = slotCount;

        SCOPED_TRACE(slotCount);
        ExpectMentions(ErrorOf(documents), {GamedataFile::INVENTORY_SECTIONS, "2번째 행", "slotCount"});
    }
}

// 아이템은 자기 종류의 구역에만 들어간다. 구역이 없는 종류의 아이템은 인벤토리에 넣을 곳이 없다.
TEST(GamedataParserTest, EveryItemTypeNeedsExactlyOneSection)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.inventorySections.erase(2);
    ExpectMentions(ErrorOf(documents), {GamedataFile::INVENTORY_SECTIONS, "MISCELLANEOUS"});

    documents = MakeValidDocuments();
    documents.inventorySections.push_back(MakeInventorySection("GEAR", 8));
    ExpectMentions(ErrorOf(documents), {GamedataFile::INVENTORY_SECTIONS, "4번째 행", "GEAR"});

    documents = MakeValidDocuments();
    documents.inventorySections[0]["itemType"] = "NONE";
    ExpectMentions(ErrorOf(documents), {GamedataFile::INVENTORY_SECTIONS, "1번째 행", "itemType"});
}

// 퀵슬롯 개수는 서버와 클라이언트가 함께 쓰는 값 하나다.
TEST(GamedataParserTest, QuickSlotTableNeedsExactlyOneRow)
{
    GamedataDocuments documents = MakeValidDocuments();
    documents.quickSlots = Json::array();
    ExpectMentions(ErrorOf(documents), {GamedataFile::QUICK_SLOTS, "하나"});

    documents = MakeValidDocuments();
    documents.quickSlots.push_back(MakeQuickSlot(9));
    ExpectMentions(ErrorOf(documents), {GamedataFile::QUICK_SLOTS, "하나"});
}

// DB의 QuickSlot.SlotIndex가 TINYINT라 256번째 퀵슬롯부터 저장하지 못한다.
TEST(GamedataParserTest, QuickSlotCountOutOfRangeIsRejected)
{
    for (int32 slotCount : { 0, -1, 257 })
    {
        GamedataDocuments documents = MakeValidDocuments();
        documents.quickSlots[0]["slotCount"] = slotCount;

        SCOPED_TRACE(slotCount);
        ExpectMentions(ErrorOf(documents), {GamedataFile::QUICK_SLOTS, "1번째 행", "slotCount"});
    }

    GamedataDocuments documents = MakeValidDocuments();
    documents.quickSlots[0]["slotCount"] = 256;
    EXPECT_EQ(ErrorOf(documents), "") << "0번부터 255번까지 256개는 저장할 수 있다";
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
    const filesystem::path dataDir = filesystem::path(__FILE__).parent_path() / ".." / "GameServer" / "Game" / "Data";
    const filesystem::path dir = dataDir / "Json";
    const filesystem::path generatedDir = dataDir / "Generated";

    GamedataDocuments documents;
    documents.levels = ReadJsonFile(generatedDir / DesignData::LEVEL_FILE);
    documents.classLevelStats = ReadJsonFile(generatedDir / DesignData::CLASS_LEVEL_STAT_FILE);
    documents.items = ReadJsonFile(generatedDir / DesignData::ITEM_FILE);
    documents.itemEquipStats = ReadJsonFile(generatedDir / DesignData::ITEM_EQUIP_STAT_FILE);
    documents.itemConsumeEffects = ReadJsonFile(generatedDir / DesignData::ITEM_CONSUME_EFFECT_FILE);
    documents.maps = ReadJsonFile(dir / GamedataFile::MAPS);
    documents.monsters = ReadJsonFile(generatedDir / DesignData::MONSTER_FILE);
    documents.dropGroups = ReadJsonFile(generatedDir / DesignData::DROP_GROUP_FILE);
    documents.currencies = ReadJsonFile(generatedDir / DesignData::CURRENCY_FILE);
    documents.initialProgress = ReadJsonFile(dir / GamedataFile::INITIAL_PROGRESS);
    documents.inventorySections = ReadJsonFile(dir / GamedataFile::INVENTORY_SECTIONS);
    documents.quickSlots = ReadJsonFile(dir / GamedataFile::QUICK_SLOTS);
    ASSERT_TRUE(documents.maps.is_array()) << "기획표를 찾지 못했다: " << dir.string();
    ASSERT_TRUE(documents.items.is_array()) << "생성된 기획 데이터를 찾지 못했다: " << generatedDir.string();

    GamedataTables tables;
    const optional<string> error = GamedataParser::Parse(documents, OUT tables);
    EXPECT_FALSE(error.has_value()) << error.value_or("");
    EXPECT_EQ(tables.townRoomId, 10);
    ASSERT_FALSE(tables.levels.empty());
    EXPECT_EQ(tables.levels.rbegin()->first, 50);

    // DB 기본값과 코드 상수에서 옮겨 온 값이다. 옮기면서 값이 바뀌지 않았는지 본다.
    EXPECT_EQ(tables.initialProgress.startMapId, 1111);
    EXPECT_EQ(tables.initialProgress.startGold, 10000);
    for (int32 itemType : { Protocol::ITEM_TYPE_GEAR, Protocol::ITEM_TYPE_CONSUMABLE, Protocol::ITEM_TYPE_MISCELLANEOUS })
        EXPECT_EQ(tables.inventorySlotCounts.at(itemType), 32) << "클라이언트 UI가 구역마다 32칸을 고정으로 쓴다";
    EXPECT_EQ(tables.quickSlotCount, 9) << "퀵슬롯 키는 1~9다";
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
}
