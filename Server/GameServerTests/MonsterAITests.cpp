#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Monster.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "Game/AI/MonsterAIComponent.h"

/*--------------------------------------------------------------
    몬스터 AI 테스트

    몬스터는 룸 틱(Room::Tick)으로만 움직인다. 상태 전환 판정은 누적 시간이 0.2초를 채울 때마다 돌고,
    이동과 공격, 예약한 피격은 틱마다 돈다. 테스트는 타이머를 기다리지 않고 Room::Tick에 시간을 넘긴다.

    픽스처 결합도: Room::Create로 룸을 만들고 Start()는 부르지 않는다(몬스터 자동 스폰과 틱 예약을 피한다).
    몬스터는 Room::SpawnEntity로, 플레이어는 세션 없이 불러온 진행으로 만들어 EnterPlayer로 넣는다.
    몬스터 템플릿과 1레벨 전사 레벨 표를 Gamedata::Install로 주입한다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 ROOM_ID = 20;
    constexpr int32 MONSTER_TEMPLATE_ID = 5000;
    constexpr int32 PLAYER_MAX_HP = 100;
    constexpr float ATTACK_RANGE = 150.f;
    constexpr float DETECTION_RANGE = 900.f;

    // 상태 전환 판정 주기(0.2초)를 한 번에 채우는 시간.
    constexpr float STATE_UPDATE_TIME = 0.2f;
    constexpr float ROOM_TICK_TIME = 0.05f;

    PlayerProgress MakeAliveProgress()
    {
        PlayerProgress progress;
        progress.playerInfo.set_class_(Protocol::CLASS_TYPE_WARRIOR);
        progress.playerInfo.set_level(1);
        progress.playerInfo.set_room_id(ROOM_ID);

        auto* stats = progress.statInfo.mutable_info();
        for (Protocol::StatType statType : { Protocol::STAT_TYPE_EXP, Protocol::STAT_TYPE_MP,
            Protocol::STAT_TYPE_PHYSICAL_ATTACK, Protocol::STAT_TYPE_MAGICAL_ATTACK })
            (*stats)[statType] = 0;
        (*stats)[Protocol::STAT_TYPE_HP] = PLAYER_MAX_HP;
        return progress;
    }

    Protocol::PosInfo MakePos(float x, float y)
    {
        Protocol::PosInfo pos;
        pos.mutable_pos()->set_x(x);
        pos.mutable_pos()->set_y(y);
        return pos;
    }
}

class MonsterAITest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        MonsterTemplate monsterTemplate;
        monsterTemplate.templateId = MONSTER_TEMPLATE_ID;
        monsterTemplate.maxHp = 100;
        monsterTemplate.baseAttack = 10;
        monsterTemplate.attackInterval = 2.5f;
        monsterTemplate.tryAttackRange = ATTACK_RANGE;
        monsterTemplate.detectionRange = DETECTION_RANGE;
        monsterTemplate.chasingMaxRange = 1200.f;
        monsterTemplate.movementSpeed = 300.f;

        LevelTemplate level1;
        level1.level = 1;
        level1.maxHp = PLAYER_MAX_HP;

        GamedataTables tables;
        tables.monsters[MONSTER_TEMPLATE_ID] = monsterTemplate;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1 });
        Gamedata::Install(std::move(tables));

        MapTemplate mapTemplate;
        mapTemplate.templateId = ROOM_ID;
        mapTemplate.depthHalfExtent = 5000.f;
        mapTemplate.widthHalfExtent = 5000.f;
        room = Room::Create(mapTemplate);
        ASSERT_NE(room, nullptr);

        MonsterSpawnParams monsterParams;
        monsterParams.templateId = MONSTER_TEMPLATE_ID;
        monsterParams.spawnPos = MakePos(0.f, 0.f);
        monster = room->SpawnEntity<Monster>(monsterParams);
        ASSERT_NE(monster, nullptr);
    }

    void TearDown() override
    {
        player.reset();
        monster.reset();
        room.reset();
        Gamedata::Install(GamedataTables());
    }

    /** (x, y)에 살아 있는 플레이어를 넣는다. */
    void EnterPlayerAt(float x, float y)
    {
        const PlayerProgress progress = MakeAliveProgress();
        PlayerSpawnParams params;
        params.progress = &progress;
        player = EntityFactory::Create<Player>(params);
        ASSERT_NE(player, nullptr);

        RoomEnterData enterData{};
        enterData.nextRoomId = ROOM_ID;
        enterData.enterType = Protocol::ENTER_TYPE_INITIAL;
        enterData.enterPos = MakePos(x, y);
        ASSERT_TRUE(room->EnterPlayer(player, enterData));
    }

    RoomRef room;
    MonsterRef monster;
    PlayerRef player;
};

TEST_F(MonsterAITest, ChasesPlayerInDetectionRangeAfterStateUpdate)
{
    EnterPlayerAt(DETECTION_RANGE - 100.f, 0.f);

    room->Tick(ROOM_TICK_TIME);
    EXPECT_EQ(monster->GetAI().GetState(), MonsterState::Idle) << "상태 전환 판정은 0.2초가 쌓여야 돈다";

    room->Tick(STATE_UPDATE_TIME);
    EXPECT_EQ(monster->GetAI().GetState(), MonsterState::Chasing);
    EXPECT_EQ(monster->GetAI().GetTarget(), player);
}

// TD-042: 대상이 룸에 남았는지는 0.2초마다 도는 상태 전환 판정만 봤다. 그사이의 틱은 다른 룸으로 떠난 플레이어의 위치를
// 읽었다. 그 위치는 새 룸 큐가 쓰므로, 다른 룸의 오브젝트에 손대지 않는다는 불변식을 어긴다.
TEST_F(MonsterAITest, LosesTargetOnFirstTickAfterTargetLeavesRoom)
{
    EnterPlayerAt(DETECTION_RANGE - 100.f, 0.f);
    room->Tick(STATE_UPDATE_TIME);
    ASSERT_EQ(monster->GetAI().GetTarget(), player);

    ASSERT_TRUE(room->LeavePlayer(player, true));
    room->Tick(ROOM_TICK_TIME);

    EXPECT_EQ(monster->GetAI().GetTarget(), nullptr) << "다음 상태 전환 판정까지 떠난 대상의 위치를 읽는다";
    EXPECT_EQ(monster->GetAI().GetState(), MonsterState::Idle);
}
