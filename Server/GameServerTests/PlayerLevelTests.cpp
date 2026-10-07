#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"

/*--------------------------------------------------------------
    레벨 상한 테스트

    레벨은 레벨 표 안에 있어야 한다. 표 밖의 레벨로 저장되면 다음 입장에서 레벨 표
    조회가 실패해 그 캐릭터는 게임에 들어오지 못한다. 최대 레벨에서 경험치 보상을
    받아도 레벨이 오르지 않는지 확인한다.

    픽스처 결합도: Player를 세션 없이 EntityFactory로만 만들고 스탯은 손으로 넣는다. 최대 레벨에서는
    다음 레벨 데이터를 읽지 않으므로 레벨 표를 시드할 필요가 없다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 MAX_LEVEL = 50;

    Protocol::S_REWARD_RESULT MakeExpReward(int64 exp)
    {
        Protocol::S_REWARD_RESULT pkt;
        pkt.mutable_reward()->set_exp(exp);
        return pkt;
    }
}

class PlayerLevelTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        player = EntityFactory::Create<Player>(PlayerSpawnParams());
        ASSERT_NE(player, nullptr);

        player->SetStatValue(Protocol::STAT_TYPE_EXP, 90);
        player->SetStatValue(Protocol::STAT_TYPE_MAX_EXP, 100);
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

TEST_F(PlayerLevelTest, RewardBelowMaxExpAccumulates)
{
    player->_playerInfo->set_level(1);

    Protocol::S_REWARD_RESULT pkt = MakeExpReward(5);
    player->OnGetReward(pkt);

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_EXP), 95) << "레벨업하지 않는 보상의 경험치가 버려지면 안 된다";
    EXPECT_EQ(pkt.updated_exp(), 95);
    EXPECT_FALSE(pkt.is_level_up());
}

TEST_F(PlayerLevelTest, MissingMaxExpDoesNotLevelUp)
{
    // 레벨 표에 다음 레벨 행이 없으면 maxExp가 0으로 캐시된다.
    player->_playerInfo->set_level(1);
    player->SetStatValue(Protocol::STAT_TYPE_MAX_EXP, 0);

    Protocol::S_REWARD_RESULT pkt = MakeExpReward(10);
    player->OnGetReward(pkt);

    EXPECT_EQ(player->_playerInfo->level(), 1) << "maxExp가 0일 때 레벨을 올리면 보상 한 번에 최대 레벨까지 간다";
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_EXP), 100);
    EXPECT_FALSE(pkt.is_level_up());
}

TEST_F(PlayerLevelTest, MaxLevelDiscardsRewardExp)
{
    player->_playerInfo->set_level(MAX_LEVEL);
    player->SetStatValue(Protocol::STAT_TYPE_EXP, 0);

    Protocol::S_REWARD_RESULT pkt = MakeExpReward(30);
    player->OnGetReward(pkt);

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_EXP), 0);
}

/*--------------------------------------------------------------
    연속 레벨업 테스트

    한 번의 보상으로 여러 레벨을 올릴 수 있어야 한다. 레벨 표의 expRequirement는
    그 레벨에서 다음 레벨로 가는 데 필요한 경험치다.

    픽스처 결합도: 전사 레벨 표를 손으로 시드하고 Player::OnLoaded()로 다음 레벨 데이터를
    캐시한다. 레벨 표에 스탯 값을 넣지 않으므로 최종 스탯 검증이 0으로 통과한다.
---------------------------------------------------------------*/

class PlayerMultiLevelUpTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        using namespace JsonProperty::LevelTable;

        Gamedata::s_classLevelDataTableMappings[Protocol::CLASS_TYPE_WARRIOR] = &Gamedata::s_warriorLevelDataTable;
        for (int32 level = 1; level <= MAX_LEVEL; level++)
        {
            Json row;
            row[string(Level)] = level;
            row[string(ExpRequirement)] = level * 100;
            // 다음 레벨 데이터를 const로 읽으므로 키가 빠지면 assert가 난다
            for (string_view key : { MaxHp_Increment, MaxMp_Increment, PA_Increment, MA_Increment })
                row[string(key)] = 0;
            Gamedata::s_warriorLevelDataTable[level] = row;
        }

        player = EntityFactory::Create<Player>(PlayerSpawnParams());
        ASSERT_NE(player, nullptr);
        player->_playerInfo->set_class_(Protocol::CLASS_TYPE_WARRIOR);

        for (Protocol::StatType type : { Protocol::STAT_TYPE_HP, Protocol::STAT_TYPE_MP,
                                         Protocol::STAT_TYPE_PHYSICAL_ATTACK, Protocol::STAT_TYPE_MAGICAL_ATTACK,
                                         Protocol::STAT_TYPE_EXP })
            player->SetStatValue(type, 0);
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::s_warriorLevelDataTable.clear();
        Gamedata::s_classLevelDataTableMappings.clear();
    }

    void LoadAtLevel(int32 level)
    {
        player->_playerInfo->set_level(level);
        player->SetStatValue(Protocol::STAT_TYPE_MAX_EXP, level * 100);
        ASSERT_TRUE(player->OnLoaded());
    }

    PlayerRef player;
};

TEST_F(PlayerMultiLevelUpTest, LargeRewardRaisesSeveralLevelsAtOnce)
{
    LoadAtLevel(1);

    // 1→2에 100, 2→3에 200, 남는 50
    Protocol::S_REWARD_RESULT pkt = MakeExpReward(350);
    player->OnGetReward(pkt);

    EXPECT_EQ(player->_playerInfo->level(), 3);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_EXP), 50);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MAX_EXP), 300);

    ASSERT_TRUE(pkt.is_level_up());
    ASSERT_TRUE(pkt.has_level_up_details()) << "레벨업 정보가 패킷에 실리지 않으면 클라가 새 스탯을 알 수 없다";
    EXPECT_EQ(pkt.level_up_details().old_level(), 1);
    EXPECT_EQ(pkt.level_up_details().new_level(), 3);
    EXPECT_EQ(pkt.updated_exp(), 50);

    // 클라이언트에는 레벨 표가 없어 새 최대 경험치를 이 패킷으로만 안다.
    const auto& updatedStats = pkt.level_up_details().updated_stat();
    const auto maxExp = std::find_if(updatedStats.begin(), updatedStats.end(),
        [](const Protocol::Stat& stat) { return stat.type() == Protocol::STAT_TYPE_MAX_EXP; });
    ASSERT_NE(maxExp, updatedStats.end()) << "레벨업 정보에 최대 경험치가 없으면 클라의 경험치 막대가 옛 최대치로 그린다";
    EXPECT_EQ(maxExp->value(), 300);
}

TEST_F(PlayerMultiLevelUpTest, ReachingMaxLevelDropsLeftoverExp)
{
    LoadAtLevel(MAX_LEVEL - 1);

    Protocol::S_REWARD_RESULT pkt = MakeExpReward((MAX_LEVEL - 1) * 100 + 70);
    player->OnGetReward(pkt);

    EXPECT_EQ(player->_playerInfo->level(), MAX_LEVEL);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_EXP), 0) << "최대 레벨에서는 경험치를 쌓지 않는다";
    EXPECT_EQ(pkt.level_up_details().new_level(), MAX_LEVEL);
}
