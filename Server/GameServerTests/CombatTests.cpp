#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Combat/Combat.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/Monster.h"
#include "Game/Entities/EntityFactory.h"

/*--------------------------------------------------------------
    피격과 처치 판정 테스트

    사망한 크리처는 공격하지도 피격되지도 않는다. 사망한 플레이어가 리스폰할 때까지
    룸에 남기 때문이다. 처치는 플레이어의 공격으로 몬스터가 사망하는 것이고, 처치일
    때만 보상이 붙는다.

    픽스처 결합도: Gamedata::s_monsterDataTable을 손으로 시드하고 두 엔티티를 EntityFactory로
    만든다. 플레이어는 세션 없이 만들고 HP 스탯을 손으로 넣는다. 룸에는 넣지 않는다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 MONSTER_TEMPLATE_ID = 9101;
    constexpr int64 MONSTER_MAX_HP = 300;
    constexpr int64 PLAYER_HP = 500;
    constexpr int64 MIN_EXP = 10;
    constexpr int64 MAX_EXP = 20;
    constexpr int64 MIN_GOLD = 5;
    constexpr int64 MAX_GOLD = 8;

    Protocol::AttackInfo MakeAttack(int64 targetId, int64 damage)
    {
        Protocol::AttackInfo attackInfo;
        attackInfo.set_type(Protocol::ATTACK_TYPE_NORMAL);
        attackInfo.set_target_id(targetId);
        attackInfo.set_damage(damage);
        return attackInfo;
    }
}

class CombatTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        using namespace JsonProperty::Monster;

        Json data;
        data[string(TemplateId)] = MONSTER_TEMPLATE_ID;
        data[string(MaxHp)] = MONSTER_MAX_HP;
        data[string(ExpReward)][string(MinExp)] = MIN_EXP;
        data[string(ExpReward)][string(MaxExp)] = MAX_EXP;
        data[string(GoldReward)][string(MinGold)] = MIN_GOLD;
        data[string(GoldReward)][string(MaxGold)] = MAX_GOLD;
        Gamedata::s_monsterDataTable[MONSTER_TEMPLATE_ID] = data;

        MonsterSpawnParams monsterParams;
        monsterParams.templateId = MONSTER_TEMPLATE_ID;
        monster = EntityFactory::Create<Monster>(monsterParams);
        ASSERT_NE(monster, nullptr);

        player = EntityFactory::Create<Player>(PlayerSpawnParams());
        ASSERT_NE(player, nullptr);
        player->SetStatValue(Protocol::STAT_TYPE_HP, PLAYER_HP);
        player->SetStatValue(Protocol::STAT_TYPE_MAX_HP, PLAYER_HP);
    }

    void TearDown() override
    {
        monster.reset();
        player.reset();
        Gamedata::s_monsterDataTable.clear();
    }

    MonsterRef monster;
    PlayerRef player;
};

TEST_F(CombatTest, HitReducesHpByDamage)
{
    const auto result = Combat::ResolveHit(monster, player, MakeAttack(player->GetEntityId(), 100));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->updatedHp, PLAYER_HP - 100);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), PLAYER_HP - 100);
    EXPECT_FALSE(result->isDead);
    EXPECT_FALSE(player->IsDead());
}

TEST_F(CombatTest, OverkillClampsHpToZeroAndDies)
{
    const auto result = Combat::ResolveHit(monster, player, MakeAttack(player->GetEntityId(), PLAYER_HP + 1000));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->updatedHp, 0) << "HP가 0 아래로 내려가면 안 된다";
    EXPECT_TRUE(result->isDead);
    EXPECT_TRUE(player->IsDead());
}

TEST_F(CombatTest, DeadTargetIsNotHit)
{
    player->SetStatValue(Protocol::STAT_TYPE_HP, 0);
    player->OnDie(monster);

    const auto result = Combat::ResolveHit(monster, player, MakeAttack(player->GetEntityId(), 100));

    EXPECT_FALSE(result.has_value()) << "사망한 플레이어는 리스폰할 때까지 룸에 남으므로 다시 맞으면 안 된다";
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 0);
}

TEST_F(CombatTest, DeadAttackerDealsNoDamage)
{
    monster->SetStatValue(Protocol::STAT_TYPE_HP, 0);
    monster->OnDie(player);

    const auto result = Combat::ResolveHit(monster, player, MakeAttack(player->GetEntityId(), 100));

    EXPECT_FALSE(result.has_value()) << "판정을 기다리는 사이 사망한 공격자의 공격은 없던 것이 된다";
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), PLAYER_HP);
}

TEST_F(CombatTest, PlayerKillingMonsterGetsReward)
{
    const auto result = Combat::ResolveHit(player, monster, MakeAttack(monster->GetEntityId(), MONSTER_MAX_HP));

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->isDead);
    ASSERT_TRUE(result->kill.has_value());
    EXPECT_EQ(result->kill->killer, player);
    EXPECT_GE(result->kill->reward.exp(), MIN_EXP);
    EXPECT_LE(result->kill->reward.exp(), MAX_EXP);
    EXPECT_GE(result->kill->reward.gold(), MIN_GOLD);
    EXPECT_LE(result->kill->reward.gold(), MAX_GOLD);
}

TEST_F(CombatTest, MonsterKillingPlayerIsNotKill)
{
    const auto result = Combat::ResolveHit(monster, player, MakeAttack(player->GetEntityId(), PLAYER_HP));

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->isDead);
    EXPECT_FALSE(result->kill.has_value()) << "몬스터가 플레이어를 사망시키는 것은 처치가 아니다";
}

TEST_F(CombatTest, NonLethalHitHasNoReward)
{
    const auto result = Combat::ResolveHit(player, monster, MakeAttack(monster->GetEntityId(), 1));

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->isDead);
    EXPECT_FALSE(result->kill.has_value());
}

TEST_F(CombatTest, MonsterInfoHpFollowsStatAfterHit)
{
    Combat::ResolveHit(player, monster, MakeAttack(monster->GetEntityId(), 50));

    EXPECT_EQ(monster->_entityInfo->monster_info().hp(), MONSTER_MAX_HP - 50) << "나중에 들어온 플레이어가 받는 스폰 정보에 현재 HP가 실려야 한다";
}
