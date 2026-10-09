#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Entities/EntityFactory.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/Monster.h"

/*--------------------------------------------------------------
    엔티티 생성 테스트

    팩토리는 id를 발급하고 스폰 매개변수를 Init에 넘긴다. 엔티티 타입은 생성자가 정한다.
    룸에 넣고 Start하는 일은 룸이 맡으므로 여기서는 보지 않는다.

    픽스처 결합도: 몬스터 템플릿을 Gamedata::Install로 주입한다. 플레이어는 세션 없이 만든다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 MONSTER_TEMPLATE_ID = 9201;
    constexpr int32 UNKNOWN_TEMPLATE_ID = 9299;
    constexpr int64 MAX_HP = 300;

    MonsterSpawnParams MakeMonsterParams(int32 templateId)
    {
        MonsterSpawnParams params;
        params.templateId = templateId;
        params.spawnPos.mutable_pos()->set_x(100.f);
        params.spawnPos.mutable_pos()->set_y(200.f);
        return params;
    }
}

class EntityFactoryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        MonsterTemplate monster;
        monster.templateId = MONSTER_TEMPLATE_ID;
        monster.maxHp = MAX_HP;

        GamedataTables tables;
        tables.monsters[MONSTER_TEMPLATE_ID] = monster;
        Gamedata::Install(std::move(tables));
    }

    void TearDown() override
    {
        Gamedata::Install(GamedataTables());
    }
};

TEST_F(EntityFactoryTest, MonsterGetsIdTypeAndSpawnParams)
{
    MonsterRef monster = EntityFactory::Create<Monster>(MakeMonsterParams(MONSTER_TEMPLATE_ID));

    ASSERT_NE(monster, nullptr);
    EXPECT_GT(monster->GetEntityId(), 0);
    EXPECT_EQ(monster->_entityInfo->entity_type(), Protocol::ENTITY_TYPE_MONSTER);
    EXPECT_EQ(monster->GetTemplateId(), MONSTER_TEMPLATE_ID);
    EXPECT_FLOAT_EQ(monster->_posInfo->pos().x(), 100.f);
    EXPECT_FLOAT_EQ(monster->_posInfo->pos().y(), 200.f);
    EXPECT_EQ(monster->_posInfo->entity_id(), monster->GetEntityId()) << "스폰 위치를 복사하면서 위치의 엔티티 id가 지워지면 안 된다";
}

TEST_F(EntityFactoryTest, UnknownMonsterTemplateFails)
{
    EXPECT_EQ(EntityFactory::Create<Monster>(MakeMonsterParams(UNKNOWN_TEMPLATE_ID)), nullptr);
}

TEST_F(EntityFactoryTest, PlayerWithoutSessionGetsIdAndInventory)
{
    PlayerRef player = EntityFactory::Create<Player>(PlayerSpawnParams());

    ASSERT_NE(player, nullptr);
    EXPECT_GT(player->GetEntityId(), 0);
    EXPECT_EQ(player->_entityInfo->entity_type(), Protocol::ENTITY_TYPE_PLAYER);
    EXPECT_EQ(player->_posInfo->entity_id(), player->GetEntityId());
    EXPECT_NE(player->_inventory, nullptr);
    EXPECT_NE(player->_equippedGear, nullptr);
    EXPECT_EQ(player->_session.lock(), nullptr);
}

TEST_F(EntityFactoryTest, EachEntityGetsDistinctId)
{
    PlayerRef player = EntityFactory::Create<Player>(PlayerSpawnParams());
    MonsterRef monster = EntityFactory::Create<Monster>(MakeMonsterParams(MONSTER_TEMPLATE_ID));

    ASSERT_NE(player, nullptr);
    ASSERT_NE(monster, nullptr);
    EXPECT_NE(player->GetEntityId(), monster->GetEntityId());
}
