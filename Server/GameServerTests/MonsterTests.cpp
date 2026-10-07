#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Entities/Monster.h"
#include "Game/Entities/EntityUtils.h"

/*--------------------------------------------------------------
    몬스터 초기화 테스트

    피격 처리는 스탯의 HP를 읽는다. 초기화가 HP를 스탯에 넣지 않으면 첫 피격에서
    서버가 죽는다. 보상은 기획 데이터의 최솟값과 최댓값 사이에서 뽑는다.

    픽스처 결합도: Gamedata::s_monsterDataTable을 손으로 시드하고
    EntityUtils::CreateMonster로 만든다. 룸에는 넣지 않는다.
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
        using namespace JsonProperty::Monster;

        Json data;
        data[string(TemplateId)] = MONSTER_TEMPLATE_ID;
        data[string(MaxHp)] = MAX_HP;
        data[string(ExpReward)][string(MinExp)] = EXP_REWARD;
        data[string(ExpReward)][string(MaxExp)] = EXP_REWARD;
        data[string(GoldReward)][string(MinGold)] = GOLD_REWARD;
        data[string(GoldReward)][string(MaxGold)] = GOLD_REWARD;
        Gamedata::s_monsterDataTable[MONSTER_TEMPLATE_ID] = data;

        monster = EntityUtils::CreateMonster(MONSTER_TEMPLATE_ID, Protocol::PosInfo());
        ASSERT_NE(monster, nullptr);
    }

    void TearDown() override
    {
        monster.reset();
        Gamedata::s_monsterDataTable.clear();
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
