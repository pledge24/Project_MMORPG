#include "Core/pch.h"
#include <gtest/gtest.h>
#include "PlayerTestAccess.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "Game/Entities/EntityFactory.h"
#include "TestLevelTables.h"

/*--------------------------------------------------------------
    레벨과 경험치 테스트

    플레이어는 누적 경험치를 들고, 레벨은 그 값과 직업 레벨 표로 정한다. 클라이언트에는
    레벨 안의 경험치와 그 레벨의 요구치를 보내고, DB에는 누적 경험치를 저장한다.
    레벨이 레벨 표 밖으로 나가면 다음 입장에서 레벨 표 조회가 실패해 그 캐릭터는 게임에
    들어오지 못한다.

    픽스처 결합도: MAX_LEVEL행짜리 레벨 표와 전사의 능력치 표를 Gamedata::Install로 주입하고, 세션 없이
    진행 사본으로 Player를 불러온다. 불러올 때의 HP와 MP는 0이라 최종 스탯 검증을 통과한다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 MAX_LEVEL = 50;

    /** 레벨이 오를 때마다 오르는 양이 다르도록 레벨의 제곱으로 정한 능력치. */
    constexpr int32 MaxHpAt(int32 level) { return 1000 + level * level * 10; }
    constexpr int32 PhysicalAttackAt(int32 level) { return 20 + level * level; }

    /**
     * 레벨 1부터 maxLevel까지의 레벨 표와 전사의 능력치 표를 설치한다.
     * 다음 레벨로 가는 경험치는 레벨 × 100이고, 최대 레벨은 0이다.
     */
    void InstallWarriorLevelTable(int32 maxLevel)
    {
        GamedataTables tables;
        for (int32 level = 1; level <= maxLevel; level++)
        {
            DesignData::ClassLevelStatRow& stat = TestLevelTables::AddWarriorLevel(tables, level, level < maxLevel ? level * 100 : 0);
            stat.MaxHp = MaxHpAt(level);
            stat.MaxMp = 100;
            stat.PhysicalAttack = PhysicalAttackAt(level);
        }
        Gamedata::Install(std::move(tables));
    }

    /** MakeLevelTable의 표에서 level에 막 오른 누적 경험치. 100 × (1 + 2 + … + (level - 1))이다. */
    constexpr int64 TotalExpAtLevel(int32 level)
    {
        return 50LL * level * (level - 1);
    }

    Reward MakeExpReward(int64 exp)
    {
        Reward reward;
        reward.exp = exp;
        return reward;
    }

    int64 SavedTotalExp(const Player& player)
    {
        return player.MakeSaveData().progress.totalExp;
    }
}

class PlayerLevelTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        InstallWarriorLevelTable(MAX_LEVEL);
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::Install(GamedataTables());
    }

    // DB가 불러온 것처럼 누적 경험치로 플레이어를 만든다. 저장된 레벨 사본은 일부러 1로 둔다.
    void LoadWithTotalExp(int64 totalExp)
    {
        PlayerProgress progress;
        progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
        progress.level = 1;
        progress.totalExp = totalExp;

        PlayerSpawnParams params;
        params.progress = &progress;
        player = EntityFactory::Create<Player>(params);
        ASSERT_NE(player, nullptr);
    }

    PlayerRef player;
};

TEST_F(PlayerLevelTest, MaxLevelIsLastLevelOfClassTable)
{
    InstallWarriorLevelTable(3);
    player = EntityFactory::Create<Player>(PlayerSpawnParams());
    PlayerTestAccess::CharacterClass(*player) = Protocol::CHARACTER_CLASS_WARRIOR;

    PlayerTestAccess::PlayerInfo(*player).set_level(2);
    EXPECT_FALSE(player->IsMaxLevel());

    PlayerTestAccess::PlayerInfo(*player).set_level(3);
    EXPECT_TRUE(player->IsMaxLevel()) << "최대 레벨은 코드 상수가 아니라 레벨 표의 마지막 레벨이다";
}

TEST_F(PlayerLevelTest, ClassWithoutLevelStatsIsMaxLevel)
{
    player = EntityFactory::Create<Player>(PlayerSpawnParams());
    PlayerTestAccess::CharacterClass(*player) = Protocol::CHARACTER_CLASS_MAGE;
    PlayerTestAccess::PlayerInfo(*player).set_level(1);

    EXPECT_TRUE(player->IsMaxLevel()) << "직업별 능력치 표에 없는 직업은 오를 레벨이 없다";
}

TEST_F(PlayerLevelTest, LoadedStatsComeFromClassLevelStatOfComputedLevel)
{
    LoadWithTotalExp(TotalExpAtLevel(3));

    EXPECT_EQ(player->GetPlayerInfo().level(), 3);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MAX_HP), MaxHpAt(3));
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), PhysicalAttackAt(3));
}

TEST_F(PlayerLevelTest, LevelUpAddsStatDifferenceBetweenLevels)
{
    LoadWithTotalExp(0);
    // 장비처럼 기본 스탯 위에 얹힌 증감량은 레벨업 뒤에도 남아야 한다.
    constexpr int64 GEAR_MAX_HP = 50;
    player->SetStatValue(Protocol::STAT_TYPE_MAX_HP, player->GetStatValue(Protocol::STAT_TYPE_MAX_HP) + GEAR_MAX_HP);

    // 1→2에 100, 2→3에 200. 한 보상으로 두 레벨이 오른다.
    const RewardResult result = player->OnGetReward(MakeExpReward(300));

    ASSERT_EQ(player->GetPlayerInfo().level(), 3);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MAX_HP), MaxHpAt(3) + GEAR_MAX_HP) << "레벨 사이의 능력치 차이만 더한다";
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), PhysicalAttackAt(3));
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MAX_MP), 100) << "능력치가 같은 레벨 사이에서는 그대로다";

    ASSERT_TRUE(result.levelUp.has_value());
    const auto& changedStats = result.levelUp->changedStats;
    const auto maxHp = std::find_if(changedStats.begin(), changedStats.end(),
        [](const Protocol::Stat& stat) { return stat.type() == Protocol::STAT_TYPE_MAX_HP; });
    ASSERT_NE(maxHp, changedStats.end());
    EXPECT_EQ(maxHp->value(), MaxHpAt(3) + GEAR_MAX_HP);
}

TEST_F(PlayerLevelTest, RewardBelowRequirementAccumulates)
{
    LoadWithTotalExp(90);

    const RewardResult result = player->OnGetReward(MakeExpReward(5));

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_EXP), 95) << "레벨업하지 않는 보상의 경험치가 버려지면 안 된다";
    EXPECT_EQ(result.exp, 95);
    EXPECT_FALSE(result.levelUp.has_value());
    EXPECT_EQ(SavedTotalExp(*player), 95);
}

TEST_F(PlayerLevelTest, LargeRewardRaisesSeveralLevelsAtOnce)
{
    LoadWithTotalExp(0);

    // 1→2에 100, 2→3에 200, 남는 50
    const RewardResult result = player->OnGetReward(MakeExpReward(350));

    EXPECT_EQ(player->GetPlayerInfo().level(), 3);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_EXP), 50) << "클라이언트에는 누적값이 아니라 레벨 안의 경험치를 보낸다";
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MAX_EXP), 300);
    EXPECT_EQ(SavedTotalExp(*player), 350);

    ASSERT_TRUE(result.levelUp.has_value()) << "레벨업 정보가 결과에 없으면 클라가 새 스탯을 알 수 없다";
    EXPECT_EQ(result.levelUp->level, 3);
    EXPECT_EQ(result.exp, 50);

    // 클라이언트에는 레벨 표가 없어 새 최대 경험치를 이 결과로만 안다.
    const auto& updatedStats = result.levelUp->changedStats;
    const auto maxExp = std::find_if(updatedStats.begin(), updatedStats.end(),
        [](const Protocol::Stat& stat) { return stat.type() == Protocol::STAT_TYPE_MAX_EXP; });
    ASSERT_NE(maxExp, updatedStats.end()) << "레벨업 정보에 최대 경험치가 없으면 클라의 경험치 막대가 옛 최대치로 그린다";
    EXPECT_EQ(maxExp->value(), 300);
}

TEST_F(PlayerLevelTest, ReachingMaxLevelDropsLeftoverExp)
{
    LoadWithTotalExp(TotalExpAtLevel(MAX_LEVEL - 1));

    const RewardResult result = player->OnGetReward(MakeExpReward((MAX_LEVEL - 1) * 100 + 70));

    EXPECT_EQ(player->GetPlayerInfo().level(), MAX_LEVEL);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_EXP), 0) << "최대 레벨에서는 경험치를 쌓지 않는다";
    ASSERT_TRUE(result.levelUp.has_value());
    EXPECT_EQ(result.levelUp->level, MAX_LEVEL);
    EXPECT_EQ(SavedTotalExp(*player), TotalExpAtLevel(MAX_LEVEL)) << "최대 레벨에 이르고 남는 경험치는 저장하지 않는다";
}

TEST_F(PlayerLevelTest, MaxLevelDoesNotLevelUpOnReward)
{
    LoadWithTotalExp(TotalExpAtLevel(MAX_LEVEL));

    const RewardResult result = player->OnGetReward(MakeExpReward(1000));

    EXPECT_EQ(player->GetPlayerInfo().level(), MAX_LEVEL);
    EXPECT_FALSE(result.levelUp.has_value());
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_EXP), 0);
    EXPECT_EQ(SavedTotalExp(*player), TotalExpAtLevel(MAX_LEVEL));
}

/* 마을 리스폰의 사망 페널티는 그 레벨 요구치의 10%다. 레벨 안에 쌓인 경험치보다 많이 잃지 않는다. */

TEST_F(PlayerLevelTest, TownRespawnLosesTenPercentOfRequirement)
{
    // 레벨 2, 레벨 안 30, 요구치 200
    LoadWithTotalExp(TotalExpAtLevel(2) + 30);

    const RespawnResult result = player->ProcessRespawn(Protocol::RESPAWN_TYPE_TOWN, Protocol::PosInfo());

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_EXP), 10);
    EXPECT_EQ(SavedTotalExp(*player), TotalExpAtLevel(2) + 10);

    const auto exp = std::find_if(result.updatedStats.begin(), result.updatedStats.end(),
        [](const Protocol::Stat& stat) { return stat.type() == Protocol::STAT_TYPE_EXP; });
    ASSERT_NE(exp, result.updatedStats.end()) << "잃은 경험치를 알리지 않으면 클라의 경험치 막대가 그대로 남는다";
    EXPECT_EQ(exp->value(), 10) << "클라이언트에는 레벨 안의 경험치를 보낸다";
}

TEST_F(PlayerLevelTest, TownRespawnDoesNotLowerLevel)
{
    LoadWithTotalExp(TotalExpAtLevel(2) + 5);

    player->ProcessRespawn(Protocol::RESPAWN_TYPE_TOWN, Protocol::PosInfo());

    EXPECT_EQ(player->GetPlayerInfo().level(), 2);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_EXP), 0);
    EXPECT_EQ(SavedTotalExp(*player), TotalExpAtLevel(2));
}
