#include "pch.h"
#include <gtest/gtest.h>
#include "Player.h"

/*--------------------------------------------------------------
    레벨 상한 테스트

    레벨은 레벨 표 안에 있어야 한다. 표 밖의 레벨로 저장되면 다음 입장에서 레벨 표
    조회가 실패해 그 캐릭터는 게임에 들어오지 못한다. 최대 레벨에서 경험치 보상을
    받아도 레벨이 오르지 않는지 확인한다.

    픽스처 결합도: Player를 Init()만 하고 스탯은 손으로 넣는다. 최대 레벨에서는
    다음 레벨 데이터를 읽지 않으므로 레벨 표를 시드할 필요가 없다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 MAX_LEVEL = 50;
}

class PlayerLevelTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        player = make_shared<Player>();
        ASSERT_TRUE(player->Init());

        player->SetStatValue(Protocol::STAT_TYPE_EXP, 90);
        player->SetStatValue(Protocol::STAT_TYPE_MAX_EXP, 100);
    }

    Protocol::S_REWARD_RESULT MakeExpReward(int64 exp)
    {
        Protocol::S_REWARD_RESULT pkt;
        pkt.mutable_reward()->set_exp(exp);
        return pkt;
    }

    PlayerRef player;
};

TEST_F(PlayerLevelTest, MaxLevelDoesNotLevelUpOnReward)
{
    player->_playerInfo->set_level(MAX_LEVEL);

    Protocol::S_REWARD_RESULT pkt = MakeExpReward(1000);
    player->OnGetReward(pkt);

    EXPECT_EQ(player->_playerInfo->level(), MAX_LEVEL);
    EXPECT_FALSE(pkt.is_level_up());
}

TEST_F(PlayerLevelTest, OnLevelUpStopsAtMaxLevel)
{
    player->_playerInfo->set_level(MAX_LEVEL);

    player->OnLevelUp();

    EXPECT_EQ(player->_playerInfo->level(), MAX_LEVEL);
    EXPECT_TRUE(player->IsMaxLevel());
}
