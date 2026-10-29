#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Monster.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "RecordingSession.h"
#include "RoomManagerTestAccess.h"
#include "TestLevelTables.h"
#include "TestCreatureCombat.h"

/*--------------------------------------------------------------
    몬스터 스폰과 리젠 테스트

    룸은 맵의 스폰 마커마다 그 스폰 그룹의 최대 수만큼 몬스터를 마커의 반경 안에 둔다. 사망한 몬스터는 그 그룹의
    리젠 시간 뒤에 같은 마커의 반경 안에 다시 나온다. 리젠은 룸 틱이 센다.

    픽스처 결합도: 시작하지 않은 룸 하나(CreateTestRoom). 스폰 그룹과 스폰 마커를 Gamedata::Install로 넣고,
    Start 대신 SpawnInitialMonsters로 처음 채운다(룸 틱 예약을 피한다). 플레이어는 RecordingSession에 묶어 넣어
    스폰 패킷으로 몬스터를 본다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 ROOM_ID = 30;
    constexpr int32 MAP_ID = 7;
    constexpr int32 SPAWN_GROUP_ID = 3;
    constexpr int32 MELEE_ID = 5000;
    constexpr int32 RANGED_ID = 5001;
    constexpr float RESPAWN_SEC = 10.f;
    constexpr float SPAWN_X = 2000.f;
    constexpr float SPAWN_Y = -1000.f;
    constexpr float SPAWN_RADIUS = 300.f;
    constexpr int32 PLAYER_MAX_HP = 100;

    DesignData::MonsterRow MakeMonsterRow(int32 id)
    {
        DesignData::MonsterRow row;
        row.Id = id;
        row.MaxHp = 10;
        row.AttackInterval = 2.5f;
        row.TryAttackRange = 150.f;
        // 감지 범위를 0으로 둬 근처의 플레이어를 쫓지 않게 한다. 몬스터가 마커 근처에 머문다.
        row.DetectionRange = 0.f;
        row.ChasingMaxRange = 1200.f;
        row.MovementSpeed = 300.f;
        return row;
    }

    Protocol::PosInfo MakePos(float x, float y)
    {
        Protocol::PosInfo pos;
        pos.mutable_pos()->set_x(x);
        pos.mutable_pos()->set_y(y);
        return pos;
    }

    vector<Protocol::EntityInfo> SpawnedMonsters(const shared_ptr<RecordingSession>& session)
    {
        vector<Protocol::EntityInfo> monsters;
        for (const Protocol::S_SPAWN& spawnPkt : session->SentPackets<Protocol::S_SPAWN>(PKT_S_SPAWN))
            for (const Protocol::EntityInfo& entity : spawnPkt.entities())
                if (entity.has_monster_info())
                    monsters.push_back(entity);
        return monsters;
    }

    float PlaneDistanceFromSpawnPoint(const Protocol::EntityInfo& entity)
    {
        const float dx = entity.pos_info().pos().x() - SPAWN_X;
        const float dy = entity.pos_info().pos().y() - SPAWN_Y;
        return sqrtf(dx * dx + dy * dy);
    }
}

class MonsterSpawnTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        tables.monsters[MELEE_ID] = MakeMonsterRow(MELEE_ID);
        tables.monsters[RANGED_ID] = MakeMonsterRow(RANGED_ID);
        TestCreatureCombat::AddMonster(tables, MELEE_ID);
        TestCreatureCombat::AddMonster(tables, RANGED_ID);
        TestLevelTables::AddWarriorLevel(tables, 1).MaxHp = PLAYER_MAX_HP;
    }

    void TearDown() override
    {
        room.reset();
        player.reset();
        session.reset();
        Gamedata::Install(GamedataTables());
    }

    /** 스폰 그룹 하나와 그 그룹의 마커 하나를 넣고 룸을 만든다. entries는 (몬스터, 가중치)다. */
    void InstallSpawnGroup(int32 maxCount, const vector<pair<int32, int32>>& entries)
    {
        DesignData::SpawnGroupRow group;
        group.Id = SPAWN_GROUP_ID;
        group.MaxCount = maxCount;
        group.RespawnSec = static_cast<int32>(RESPAWN_SEC);
        for (const auto& [monsterId, weight] : entries)
        {
            DesignData::SpawnEntryRow entry;
            entry.MonsterId = monsterId;
            entry.Weight = weight;
            group.Entries.push_back(entry);
        }
        tables.spawnGroups[SPAWN_GROUP_ID] = group;
        tables.spawnPoints[MAP_ID] = { SpawnPointPlacement{ SPAWN_GROUP_ID, vector3D(SPAWN_X, SPAWN_Y, 0.f), SPAWN_RADIUS } };
        Gamedata::Install(tables);

        room = CreateTestRoom(ROOM_ID, MAP_ID, 5000.f);
        ASSERT_NE(room, nullptr);
    }

    /** 스폰 마커 옆에 플레이어를 넣고, 입장 응답으로 받은 몬스터를 돌려준다. 그 뒤로 받는 패킷만 남긴다. */
    vector<Protocol::EntityInfo> EnterPlayer()
    {
        session = make_shared<RecordingSession>();
        PlayerProgress progress;
        progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
        progress.level = 1;
        progress.hp = PLAYER_MAX_HP;
        PlayerSpawnParams params;
        params.session = session;
        params.progress = &progress;
        player = EntityFactory::Create<Player>(params);

        RoomEnterData enterData{};
        enterData.nextRoomId = ROOM_ID;
        enterData.enterPos = MakePos(SPAWN_X + 500.f, SPAWN_Y);
        EXPECT_TRUE(room->EnterPlayer(player, enterData));

        vector<Protocol::EntityInfo> monsters = SpawnedMonsters(session);
        session->ClearSent();
        return monsters;
    }

    void Kill(int64 monsterId)
    {
        AttackInfo attackInfo;
        attackInfo.targetId = monsterId;
        attackInfo.damage = 1000;
        room->HandleHit(player, attackInfo);
        ASSERT_FALSE(room->Contains(monsterId)) << "준비: 몬스터가 죽어 룸에서 빠진다";
    }

    GamedataTables tables;
    RoomRef room;
    PlayerRef player;
    shared_ptr<RecordingSession> session;
};

TEST_F(MonsterSpawnTest, InitialSpawnFillsMaxCountInsideRadius)
{
    InstallSpawnGroup(4, { { MELEE_ID, 1 } });

    ASSERT_TRUE(room->SpawnInitialMonsters());
    const vector<Protocol::EntityInfo> monsters = EnterPlayer();

    ASSERT_EQ(monsters.size(), 4u) << "마커 하나에 그룹의 최대 수만큼 둔다";
    for (const Protocol::EntityInfo& monster : monsters)
    {
        EXPECT_EQ(monster.monster_info().template_id(), MELEE_ID);
        EXPECT_LE(PlaneDistanceFromSpawnPoint(monster), SPAWN_RADIUS) << "마커의 반경 안에 둔다";
    }
}

TEST_F(MonsterSpawnTest, MonsterKindFollowsWeight)
{
    // 가중치가 0인 종류는 DataBuilder가 막는다. 여기서는 한쪽 가중치를 크게 두고 두 종류가 모두 나오는지 본다.
    InstallSpawnGroup(200, { { MELEE_ID, 3 }, { RANGED_ID, 1 } });

    ASSERT_TRUE(room->SpawnInitialMonsters());
    int32 meleeCount = 0;
    int32 rangedCount = 0;
    for (const Protocol::EntityInfo& monster : EnterPlayer())
        (monster.monster_info().template_id() == MELEE_ID ? meleeCount : rangedCount)++;

    EXPECT_EQ(meleeCount + rangedCount, 200);
    EXPECT_GT(rangedCount, 0) << "가중치가 작은 종류도 나온다";
    EXPECT_GT(meleeCount, rangedCount) << "가중치가 큰 종류가 더 많이 나온다";
}

TEST_F(MonsterSpawnTest, KilledMonsterRespawnsAfterRespawnTimeInsideRadius)
{
    InstallSpawnGroup(1, { { MELEE_ID, 1 } });
    ASSERT_TRUE(room->SpawnInitialMonsters());
    const vector<Protocol::EntityInfo> monsters = EnterPlayer();
    ASSERT_EQ(monsters.size(), 1u);

    Kill(monsters[0].entity_id());
    session->ClearSent();

    room->Tick(RESPAWN_SEC - 0.5f);
    EXPECT_TRUE(SpawnedMonsters(session).empty()) << "리젠 시간 전에는 나오지 않는다";

    room->Tick(0.5f);
    const vector<Protocol::EntityInfo> respawned = SpawnedMonsters(session);
    ASSERT_EQ(respawned.size(), 1u) << "리젠 시간 뒤에 다시 나오고 근처의 플레이어가 스폰을 받는다";
    EXPECT_NE(respawned[0].entity_id(), monsters[0].entity_id()) << "새 엔티티다";
    EXPECT_TRUE(room->Contains(respawned[0].entity_id()));
    EXPECT_LE(PlaneDistanceFromSpawnPoint(respawned[0]), SPAWN_RADIUS) << "같은 마커의 반경 안에 나온다";
    EXPECT_EQ(respawned[0].monster_info().hp(), 10) << "HP가 가득 찬 채로 나온다";
}

TEST_F(MonsterSpawnTest, RespawnDoesNotExceedMaxCount)
{
    InstallSpawnGroup(2, { { MELEE_ID, 1 } });
    ASSERT_TRUE(room->SpawnInitialMonsters());
    const vector<Protocol::EntityInfo> monsters = EnterPlayer();
    ASSERT_EQ(monsters.size(), 2u);

    Kill(monsters[0].entity_id());
    session->ClearSent();
    room->Tick(RESPAWN_SEC);
    room->Tick(RESPAWN_SEC);

    EXPECT_EQ(SpawnedMonsters(session).size(), 1u) << "죽은 수만큼만 다시 나온다";
}

// 지역 표의 몬스터 리스폰 시간은 서버로 갔지만 읽는 코드가 없었다. 처치할 때마다 룸의 몬스터 수가 줄었다.
TEST_F(MonsterSpawnTest, MonsterSpawnedOutsideSpawnPointIsNotRespawned)
{
    InstallSpawnGroup(0, { { MELEE_ID, 1 } });
    MonsterSpawnParams params;
    params.templateId = MELEE_ID;
    params.spawnPos = MakePos(SPAWN_X, SPAWN_Y);
    MonsterRef loose = room->SpawnEntity<Monster>(params);
    ASSERT_NE(loose, nullptr);
    EnterPlayer();

    Kill(loose->GetEntityId());
    session->ClearSent();
    room->Tick(RESPAWN_SEC);

    EXPECT_TRUE(SpawnedMonsters(session).empty()) << "마커에서 나오지 않은 몬스터는 리젠할 마커가 없다";
}
