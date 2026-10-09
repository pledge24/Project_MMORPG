#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Entities/Monster.h"
#include "Game/Entities/EntityFactory.h"

/*--------------------------------------------------------------
    몬스터 초기화 테스트

    피격 처리는 스탯의 HP를 읽는다. 초기화가 HP를 스탯에 넣지 않으면 첫 피격에서
    서버가 죽는다. 보상은 기획 데이터의 최솟값과 최댓값 사이에서 뽑는다.

    픽스처 결합도: 몬스터 템플릿을 Gamedata::Install로 주입하고
    EntityFactory로 만든다. 룸에는 넣지 않는다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 MONSTER_TEMPLATE_ID = 9001;
    constexpr int32 MAX_HP = 300;
    constexpr int64 EXP_REWARD = 40;
    constexpr int64 GOLD_REWARD = 15;
}

class MonsterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        MonsterTemplate data;
        data.templateId = MONSTER_TEMPLATE_ID;
        data.maxHp = MAX_HP;
        data.minExp = EXP_REWARD;
        data.maxExp = EXP_REWARD;
        data.minGold = GOLD_REWARD;
        data.maxGold = GOLD_REWARD;

        GamedataTables tables;
        tables.monsters[MONSTER_TEMPLATE_ID] = data;
        Gamedata::Install(std::move(tables));

        MonsterSpawnParams spawnParams;
        spawnParams.templateId = MONSTER_TEMPLATE_ID;
        monster = EntityFactory::Create<Monster>(spawnParams);
        ASSERT_NE(monster, nullptr);
    }

    void TearDown() override
    {
        monster.reset();
        Gamedata::Install(GamedataTables());
    }

    MonsterRef monster;
};

TEST_F(MonsterTest, InitPutsHpIntoStats)
{
    ASSERT_TRUE(monster->HasStat(Protocol::STAT_TYPE_HP)) << "HP 스탯이 없으면 첫 피격에서 Map::at이 실패한다";
    EXPECT_EQ(monster->GetStatValue(Protocol::STAT_TYPE_HP), MAX_HP);
    EXPECT_EQ(monster->GetStatValue(Protocol::STAT_TYPE_MAX_HP), MAX_HP);
}

TEST_F(MonsterTest, RewardWithSameMinAndMaxReturnsThatValue)
{
    EXPECT_EQ(monster->GetExpReward(), EXP_REWARD);
    EXPECT_EQ(monster->GetGoldReward(), GOLD_REWARD);
}
