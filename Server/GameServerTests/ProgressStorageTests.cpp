#include "Core/pch.h"
#include <gtest/gtest.h>
#include "FakeDBConnection.h"
#include "PlayerTestAccess.h"
#include "DB/CharacterStateDAO.h"
#include "DB/DAOCommon.h"
#include "DB/ItemDAO.h"
#include "DB/ItemRows.h"
#include "DB/ProgressStorage.h"
#include "Network/GameEntry.h"
#include "Network/GameSessionManager.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerSaveData.h"

/*--------------------------------------------------------------
    진행 저장소와 DAO 테스트

    DAO는 연결을 인자로 받는다. 여기서는 ODBC 대신 FakeDBConnection을 넘겨, DAO가 바인딩한
    파라미터와 읽은 행을 확인한다. SQL 문장 자체가 맞는지는 이 테스트가 보지 않는다(실제 DB 테스트가 본다).

    저장 한 번은 트랜잭션 하나다. 중간 단계가 실패하면 앞 단계까지 되돌려야 레벨은 오르고 아이템은
    저장되지 않는 식으로 진행이 어긋나지 않는다.

    픽스처 결합도: FakeDBConnection과, 골드 재화 번호, 맵 표(사냥터와 마을), 레벨 표, 아이템 표, 인벤토리 구역 표를
    Gamedata::Install로 주입한다. 입장 테스트는 소켓 없는 GameSession을 만든다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 USER_ID = 7;
    constexpr int64 CHARACTER_ID = 42;
    constexpr int32 GOLD_CURRENCY_ID = 3;
    constexpr int32 FIELD_MAP_ID = 2000;
    constexpr int32 FIELD_ROOM_ID = 20;
    constexpr int32 TOWN_MAP_ID = 1000;
    constexpr int32 TOWN_ROOM_ID = 10;
    constexpr float TOWN_RESPAWN_X = 300.f;
    constexpr int32 MAX_HP = 500;
    constexpr int32 POTION_TEMPLATE_ID = 2000;
    constexpr int32 SWORD_TEMPLATE_ID = 1001;

    // CharacterStateDAO::LoadCharacter가 읽는 열 순서의 행.
    FakeDBRow MakeCharacterRow(int64 level, int64 exp, int64 mapId, double posX, double posY, int64 gold)
    {
        return { int64(Protocol::CLASS_TYPE_WARRIOR), wstring(L"전사"), level, exp, mapId, posX, posY, 30.0, 90.0,
            int64(MAX_HP), int64(80), gold };
    }

    // ItemDAO::LoadItems가 읽는 열 순서의 행.
    FakeDBRow MakeItemRow(int64 itemUid, int64 templateId, Protocol::ItemType itemType, ItemContainer container, int64 slotIndex,
        int64 stackCount, int64 enhanceLevel = 0)
    {
        return { itemUid, templateId, int64(itemType), int64(container), slotIndex, stackCount, enhanceLevel, int64(0) };
    }

    // 저장 DAO가 읽는 값이 모두 있는 사본. 아이템은 없다.
    PlayerSaveData MakeSaveData()
    {
        PlayerSaveData data;
        data.userId = USER_ID;
        data.progress.playerInfo.set_character_id(CHARACTER_ID);
        data.progress.playerInfo.set_level(3);
        data.progress.playerInfo.set_map_id(FIELD_MAP_ID);
        data.progress.playerInfo.set_room_id(FIELD_ROOM_ID);
        data.progress.posInfo.mutable_pos()->set_x(-150.f);
        data.progress.posInfo.mutable_pos()->set_y(40.f);
        data.progress.posInfo.mutable_pos()->set_z(5.f);
        data.progress.posInfo.set_yaw(45.f);
        data.progress.possession.set_gold(1234);

        auto* stats = data.progress.statInfo.mutable_info();
        (*stats)[Protocol::STAT_TYPE_EXP] = 777;
        (*stats)[Protocol::STAT_TYPE_HP] = 60;
        (*stats)[Protocol::STAT_TYPE_MP] = 70;
        return data;
    }

    void AddConsumable(PlayerSaveData& data, int64 itemUid, int32 slotId)
    {
        Protocol::Slot* slot = data.progress.possession.mutable_inventory()->add_consumables();
        slot->set_type(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE);
        slot->set_slot_id(slotId);
        slot->mutable_item()->set_item_uid(itemUid);
        slot->mutable_item()->set_template_id(POTION_TEMPLATE_ID);
        slot->mutable_item()->set_count(1);
    }
}

class ProgressStorageTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        MapTemplate field;
        field.templateId = FIELD_ROOM_ID;
        field.mapId = FIELD_MAP_ID;
        field.depthHalfExtent = 1000.f;
        field.widthHalfExtent = 1000.f;

        MapTemplate town;
        town.templateId = TOWN_ROOM_ID;
        town.mapId = TOWN_MAP_ID;
        town.center = TemplatePos{ 0.f, 5000.f, 0.f };
        town.depthHalfExtent = 1000.f;
        town.widthHalfExtent = 1000.f;
        town.respawnPoint = TemplatePos{ TOWN_RESPAWN_X, 5000.f, 0.f };

        LevelTemplate level1;
        level1.level = 1;
        level1.maxHp = MAX_HP;
        level1.maxMp = 100;
        level1.expRequirement = 1000;
        LevelTemplate level2 = level1;
        level2.level = 2;
        level2.expRequirement = 0;

        ItemTemplate potion;
        potion.templateId = POTION_TEMPLATE_ID;
        potion.itemType = Protocol::ITEM_TYPE_CONSUMABLE;
        potion.maxStack = 99;

        ItemTemplate sword;
        sword.templateId = SWORD_TEMPLATE_ID;
        sword.itemType = Protocol::ITEM_TYPE_GEAR;
        sword.gearType = Protocol::GEAR_TYPE_WEAPON;

        GamedataTables tables;
        tables.goldCurrencyId = GOLD_CURRENCY_ID;
        tables.maps[FIELD_ROOM_ID] = field;
        tables.maps[TOWN_ROOM_ID] = town;
        tables.townRoomId = TOWN_ROOM_ID;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1, level2 });
        tables.items[POTION_TEMPLATE_ID] = potion;
        tables.items[SWORD_TEMPLATE_ID] = sword;
        tables.inventorySlotCounts = MakeInventorySections();
        Gamedata::Install(std::move(tables));

        session = make_shared<GameSession>();
        sessionManager.RegisterUser(USER_ID, session);
    }

    void TearDown() override
    {
        session.reset();
        Gamedata::Install(GamedataTables());
    }

    GameSessionManager sessionManager;
    GameSessionRef session;
};

/* 불러오기 */

TEST_F(ProgressStorageTest, LoadCharacterReadsCharacterStateAndGold)
{
    FakeDBConnection conn;
    conn.QueueResult({ MakeCharacterRow(3, 2500, FIELD_MAP_ID, -150.0, 40.0, 6'000'000'000) });

    PlayerProgress progress;
    ASSERT_TRUE(CharacterStateDAO::LoadCharacter(conn, USER_ID, CHARACTER_ID, OUT progress));

    EXPECT_EQ(progress.playerInfo.character_id(), CHARACTER_ID);
    EXPECT_EQ(progress.playerInfo.class_(), Protocol::CLASS_TYPE_WARRIOR);
    EXPECT_EQ(progress.playerInfo.level(), 3);
    EXPECT_FALSE(progress.playerInfo.name().empty());
    EXPECT_EQ(progress.playerInfo.map_id(), FIELD_MAP_ID);
    EXPECT_FLOAT_EQ(progress.posInfo.pos().x(), -150.f);
    EXPECT_FLOAT_EQ(progress.posInfo.pos().y(), 40.f);
    EXPECT_FLOAT_EQ(progress.posInfo.pos().z(), 30.f);
    EXPECT_FLOAT_EQ(progress.posInfo.yaw(), 90.f);
    EXPECT_EQ(progress.statInfo.info().at(Protocol::STAT_TYPE_EXP), 2500) << "경험치는 누적값 그대로 싣는다";
    EXPECT_EQ(progress.statInfo.info().at(Protocol::STAT_TYPE_HP), MAX_HP);
    EXPECT_EQ(progress.statInfo.info().at(Protocol::STAT_TYPE_MP), 80);
    EXPECT_EQ(progress.statInfo.info().count(Protocol::STAT_TYPE_PHYSICAL_ATTACK), 0u) << "공격력은 DB에 없다";
    EXPECT_EQ(progress.possession.gold(), 6'000'000'000) << "골드는 재화 행에서 온다";

    ASSERT_EQ(conn.executedParams.size(), 1u);
    EXPECT_EQ(get<int64>(conn.executedParams[0].at(0)), CHARACTER_ID);
    EXPECT_EQ(get<int64>(conn.executedParams[0].at(1)), USER_ID) << "클라이언트가 보낸 캐릭터 번호를 믿지 않고 계정과 함께 대조한다";
    EXPECT_EQ(get<int64>(conn.executedParams[0].at(2)), GOLD_CURRENCY_ID) << "골드는 재화 표의 GOLD 재화 번호로 찾는다";
}

TEST_F(ProgressStorageTest, LoadCharacterOfOtherAccountFails)
{
    FakeDBConnection conn;
    conn.QueueResult({});

    PlayerProgress progress;
    EXPECT_FALSE(CharacterStateDAO::LoadCharacter(conn, USER_ID, CHARACTER_ID, OUT progress)) << "행이 없으면 다른 계정의 캐릭터다";
}

TEST_F(ProgressStorageTest, CharacterWithoutGoldRowIsDBError)
{
    FakeDBConnection conn;
    conn.QueueResult({ MakeCharacterRow(1, 0, FIELD_MAP_ID, 0.0, 0.0, -1) });

    PlayerProgress progress;
    EXPECT_THROW(CharacterStateDAO::LoadCharacter(conn, USER_ID, CHARACTER_ID, OUT progress), DBError)
        << "골드를 0으로 보고 입장시키면 다음 저장이 갱신할 행을 찾지 못해 실패한다";
}

TEST_F(ProgressStorageTest, LoadItemsPlacesRowsByContainerAndSection)
{
    FakeDBConnection conn;
    conn.QueueResult({
        MakeItemRow(11, POTION_TEMPLATE_ID, Protocol::ITEM_TYPE_CONSUMABLE, ItemContainer::INVENTORY, 5, 3),
        MakeItemRow(12, SWORD_TEMPLATE_ID, Protocol::ITEM_TYPE_GEAR, ItemContainer::EQUIPPED, Protocol::GEAR_TYPE_WEAPON, 1, 4),
    });

    PlayerProgress progress;
    ItemDAO::LoadItems(conn, CHARACTER_ID, OUT progress);

    ASSERT_EQ(progress.possession.inventory().consumables().size(), 1);
    const Protocol::Slot& potion = progress.possession.inventory().consumables(0);
    EXPECT_EQ(potion.slot_id(), 5);
    EXPECT_EQ(potion.item().item_uid(), 11);
    EXPECT_EQ(potion.item().count(), 3);

    ASSERT_EQ(progress.possession.equipped_gear().count(Protocol::GEAR_TYPE_WEAPON), 1u);
    const Protocol::Item& sword = progress.possession.equipped_gear().at(Protocol::GEAR_TYPE_WEAPON).item();
    EXPECT_EQ(sword.item_uid(), 12);
    EXPECT_EQ(sword.gearinfo().enhance_level(), 4);

    ASSERT_EQ(conn.executedParams.size(), 1u);
    EXPECT_EQ(get<int64>(conn.executedParams[0].at(0)), CHARACTER_ID);
}

TEST_F(ProgressStorageTest, ItemRowInUnknownPlaceIsDBError)
{
    FakeDBConnection conn;
    conn.QueueResult({ MakeItemRow(11, POTION_TEMPLATE_ID, Protocol::ITEM_TYPE_CONSUMABLE, static_cast<ItemContainer>(9), 0, 1) });

    PlayerProgress progress;
    EXPECT_THROW(ItemDAO::LoadItems(conn, CHARACTER_ID, OUT progress), DBError)
        << "둘 곳을 모르는 행을 건너뛰면 다음 저장이 그 행을 지운다";
}

TEST_F(ProgressStorageTest, FailedLoadLeavesSessionWithoutPlayer)
{
    FakeDBConnection conn;
    conn.QueueExecuteFailure();

    const Protocol::S_ENTER_GAME pkt = GameEntry::Enter(conn, session, USER_ID, CHARACTER_ID);

    EXPECT_FALSE(pkt.success());
    EXPECT_EQ(session->GetPlayer(), nullptr) << "불러오기에 실패한 세션에 플레이어가 남으면 끊길 때 반쯤 채운 진행을 저장한다";
}

/* 입장할 룸 고르기: DB는 맵과 좌표만 안다 */

TEST_F(ProgressStorageTest, EnterGoesToRoomCoveringSavedPosition)
{
    FakeDBConnection conn;
    conn.QueueResult({ MakeCharacterRow(1, 0, FIELD_MAP_ID, -150.0, 40.0, 100) });

    const Protocol::S_ENTER_GAME pkt = GameEntry::Enter(conn, session, USER_ID, CHARACTER_ID);

    ASSERT_TRUE(pkt.success());
    PlayerRef player = session->GetPlayer();
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->GetEnteringRoomId(), FIELD_ROOM_ID);
    EXPECT_EQ(pkt.player().player_info().map_id(), FIELD_MAP_ID);
    EXPECT_EQ(pkt.player().player_info().room_id(), FIELD_ROOM_ID);
    EXPECT_FLOAT_EQ(player->GetPosInfo().pos().x(), -150.f);
}

TEST_F(ProgressStorageTest, EnterOutsideEveryRoomGoesToTownRespawn)
{
    FakeDBConnection conn;
    conn.QueueResult({ MakeCharacterRow(1, 0, FIELD_MAP_ID, 50'000.0, 0.0, 100) });

    const Protocol::S_ENTER_GAME pkt = GameEntry::Enter(conn, session, USER_ID, CHARACTER_ID);

    ASSERT_TRUE(pkt.success());
    PlayerRef player = session->GetPlayer();
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->GetEnteringRoomId(), TOWN_ROOM_ID);
    EXPECT_EQ(pkt.player().player_info().map_id(), TOWN_MAP_ID) << "마을로 보내면 맵도 마을의 맵이다";
    EXPECT_FLOAT_EQ(player->GetPosInfo().pos().x(), TOWN_RESPAWN_X);
}

TEST_F(ProgressStorageTest, EnterWithoutRoomToAssignIsRejected)
{
    GamedataTables tables;
    tables.goldCurrencyId = GOLD_CURRENCY_ID;
    tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ LevelTemplate{ 1, MAX_HP, 100 } });
    Gamedata::Install(std::move(tables));

    FakeDBConnection conn;
    conn.QueueResult({ MakeCharacterRow(1, 0, FIELD_MAP_ID, -150.0, 40.0, 100) });

    const Protocol::S_ENTER_GAME pkt = GameEntry::Enter(conn, session, USER_ID, CHARACTER_ID);

    EXPECT_FALSE(pkt.success()) << "맵 표에 그 좌표를 덮는 룸도 마을 리스폰 지점도 없다";
    EXPECT_EQ(session->GetPlayer(), nullptr);
}

/* 저장 */

TEST_F(ProgressStorageTest, SaveCommitsOneTransaction)
{
    FakeDBConnection conn;

    EXPECT_TRUE(ProgressStorage::Save(conn, MakeSaveData()));

    EXPECT_EQ(conn.beginCount, 1);
    EXPECT_EQ(conn.commitCount, 1);
    EXPECT_EQ(conn.rollbackCount, 0);
}

TEST_F(ProgressStorageTest, SaveBindsLevelStateAndGold)
{
    FakeDBConnection conn;

    ASSERT_TRUE(ProgressStorage::Save(conn, MakeSaveData()));

    ASSERT_GE(conn.executedParams.size(), 3u);
    const auto& level = conn.executedParams[0];
    EXPECT_EQ(get<int64>(level.at(0)), 3);
    EXPECT_EQ(get<int64>(level.at(1)), CHARACTER_ID);

    const auto& state = conn.executedParams[1];
    EXPECT_EQ(get<int64>(state.at(0)), 777) << "누적 경험치";
    EXPECT_EQ(get<int64>(state.at(1)), FIELD_MAP_ID) << "룸이 아니라 맵을 저장한다";
    EXPECT_DOUBLE_EQ(get<double>(state.at(2)), -150.0);
    EXPECT_DOUBLE_EQ(get<double>(state.at(3)), 40.0);
    EXPECT_DOUBLE_EQ(get<double>(state.at(4)), 5.0);
    EXPECT_DOUBLE_EQ(get<double>(state.at(5)), 45.0);
    EXPECT_EQ(get<int64>(state.at(6)), 60);
    EXPECT_EQ(get<int64>(state.at(7)), 70);
    EXPECT_EQ(get<int64>(state.at(8)), CHARACTER_ID);

    const auto& gold = conn.executedParams[2];
    EXPECT_EQ(get<int64>(gold.at(0)), 1234);
    EXPECT_EQ(get<int64>(gold.at(1)), CHARACTER_ID);
    EXPECT_EQ(get<int64>(gold.at(2)), GOLD_CURRENCY_ID);
}

// 배열 파라미터 한 번에 묶을 수 있는 행 수를 넘는 아이템도 저장한다. 나눠 넣고 MERGE는 한 번만 실행해야
// 사본에 없는 행을 가려낼 수 있다. 묶음마다 MERGE하면 앞 묶음의 행을 사본에 없는 행으로 보고 지운다.
TEST_F(ProgressStorageTest, ItemsBeyondParamRowLimitAreInsertedInChunksAndMergedOnce)
{
    PlayerSaveData data = MakeSaveData();
    const int32 itemCount = MAX_PARAM_ROWS + 50;
    for (int32 i = 0; i < itemCount; i++)
        AddConsumable(data, 1000 + i, i);

    FakeDBConnection conn;
    ASSERT_TRUE(ProgressStorage::Save(conn, data));

    // 레벨, 상태, 골드, 임시 테이블, 넣기 두 번, MERGE 한 번.
    ASSERT_EQ(conn.executedQueries.size(), 7u);
    EXPECT_NE(conn.executedQueries[3].find(L"CREATE TABLE #ItemSource"), wstring::npos);
    EXPECT_NE(conn.executedQueries[4].find(L"INSERT INTO #ItemSource"), wstring::npos);
    EXPECT_NE(conn.executedQueries[5].find(L"INSERT INTO #ItemSource"), wstring::npos);
    EXPECT_EQ(get<int64>(conn.executedParams[4].at(0)), 1000) << "첫 묶음은 첫 행부터";
    EXPECT_EQ(get<int64>(conn.executedParams[5].at(0)), 1000 + MAX_PARAM_ROWS) << "둘째 묶음은 첫 묶음 다음 행부터";
    EXPECT_NE(conn.executedQueries[6].find(L"MERGE"), wstring::npos);
    EXPECT_EQ(get<int64>(conn.executedParams[6].at(0)), CHARACTER_ID) << "그 캐릭터의 행만 맞춘다";
}

TEST_F(ProgressStorageTest, SaveWithoutItemsStillMergesToDeleteRows)
{
    FakeDBConnection conn;
    ASSERT_TRUE(ProgressStorage::Save(conn, MakeSaveData()));

    // 레벨, 상태, 골드, 임시 테이블, MERGE. 넣을 행이 없어도 MERGE는 돌아야 DB에 남은 행이 지워진다.
    ASSERT_EQ(conn.executedQueries.size(), 5u);
    EXPECT_NE(conn.executedQueries[4].find(L"MERGE"), wstring::npos);
}

TEST_F(ProgressStorageTest, ItemWithoutUidFailsAndRollsBack)
{
    PlayerSaveData data = MakeSaveData();
    AddConsumable(data, 0, 0);

    FakeDBConnection conn;
    EXPECT_FALSE(ProgressStorage::Save(conn, data)) << "UID가 없으면 DB의 어느 행과 같은 아이템인지 알 수 없다";
    EXPECT_EQ(conn.rollbackCount, 1);
    EXPECT_EQ(conn.commitCount, 0);
}

TEST_F(ProgressStorageTest, MissingStateRowRollsBack)
{
    FakeDBConnection conn;
    conn.QueueResult({});
    conn.QueueResult({}, 0);

    EXPECT_FALSE(ProgressStorage::Save(conn, MakeSaveData())) << "갱신한 행이 없으면 저장이 사라진 것이다";
    EXPECT_EQ(conn.rollbackCount, 1);
}

TEST_F(ProgressStorageTest, UnexpectedExceptionRollsBack)
{
    FakeDBConnection conn;
    conn.QueueResult({});
    conn.QueueExecuteException();

    EXPECT_FALSE(ProgressStorage::Save(conn, MakeSaveData()));
    EXPECT_EQ(conn.rollbackCount, 1) << "열린 트랜잭션을 둔 채 연결을 풀에 돌려주면 다음 잡이 그 위에서 돈다";
}

// 아이템 저장이 실패하면 앞서 실행한 레벨, 상태, 골드의 갱신도 되돌린다.
TEST_F(ProgressStorageTest, FailedItemSaveRollsBackEarlierSteps)
{
    FakeDBConnection conn;
    conn.QueueResult({});
    conn.QueueResult({});
    conn.QueueResult({});
    conn.QueueExecuteFailure();

    EXPECT_FALSE(ProgressStorage::Save(conn, MakeSaveData()));

    EXPECT_EQ(conn.executedQueries.size(), 4u) << "레벨, 상태, 골드는 이미 실행됐다";
    EXPECT_EQ(conn.commitCount, 0);
    EXPECT_EQ(conn.rollbackCount, 1) << "되돌리지 않으면 레벨은 오르고 아이템은 저장되지 않은 채로 남는다";
}
