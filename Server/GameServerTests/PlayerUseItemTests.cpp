#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Inventory/InventoryComponent.h"

/*--------------------------------------------------------------
    소모품 사용 테스트

    소모품 가방에 든 아이템만 쓸 수 있다. 회복량은 요청이 아니라 슬롯에 든 아이템으로 정하고,
    요청의 아이템이 슬롯과 다르면 쓰지 않는다.
    HP나 MP가 가득 차 있어도 소비한다. 재사용 대기는 템플릿마다 따로 돌고 서버가 판정한다.

    픽스처 결합도: 아이템 템플릿을 Gamedata::Install로 주입하고 Player를 세션 없이 EntityFactory로만 만든다.
    시각은 ProcessUseItem의 인자로 넘긴다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 HP_POTION_TEMPLATE_ID = 2000;
    constexpr int32 MP_POTION_TEMPLATE_ID = 2001;
    constexpr int32 MIX_POTION_TEMPLATE_ID = 2002;
    constexpr int32 SWORD_TEMPLATE_ID = 1001;

    constexpr int64 MAX_HP = 1000;
    constexpr int64 MAX_MP = 500;
    constexpr int32 COOLDOWN_SECONDS = 10;
    constexpr uint64 NOW_MS = 100'000;

    ItemTemplate MakePotion(int32 templateId, double hpRestore, double mpRestore)
    {
        ItemTemplate potion;
        potion.templateId = templateId;
        potion.itemType = Protocol::ITEM_TYPE_CONSUMABLE;
        potion.maxStack = 10;
        potion.cooldownMs = COOLDOWN_SECONDS * 1000;
        potion.hpRestoreRatio = hpRestore;
        potion.mpRestoreRatio = mpRestore;
        return potion;
    }
}

class PlayerUseItemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        tables.items[HP_POTION_TEMPLATE_ID] = MakePotion(HP_POTION_TEMPLATE_ID, 0.3, 0);
        tables.items[MP_POTION_TEMPLATE_ID] = MakePotion(MP_POTION_TEMPLATE_ID, 0, 0.3);
        tables.items[MIX_POTION_TEMPLATE_ID] = MakePotion(MIX_POTION_TEMPLATE_ID, 0.4, 0.4);

        ItemTemplate sword;
        sword.templateId = SWORD_TEMPLATE_ID;
        sword.itemType = Protocol::ITEM_TYPE_GEAR;
        sword.gearType = Protocol::GEAR_TYPE_WEAPON;
        sword.hpRestoreRatio = 1.0;
        tables.items[SWORD_TEMPLATE_ID] = sword;
        Gamedata::Install(tables);

        player = EntityFactory::Create<Player>(PlayerSpawnParams());
        ASSERT_NE(player, nullptr);
        player->SetStatValue(Protocol::STAT_TYPE_MAX_HP, MAX_HP);
        player->SetStatValue(Protocol::STAT_TYPE_HP, 100);
        player->SetStatValue(Protocol::STAT_TYPE_MAX_MP, MAX_MP);
        player->SetStatValue(Protocol::STAT_TYPE_MP, 100);
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::Install(GamedataTables());
    }

    // 아이템을 넣고 그 슬롯을 요청 형태로 돌려준다.
    Protocol::Slot AddAndGetSlot(int32 templateId, int32 count)
    {
        RepeatedPtrField<Protocol::Slot> addedSlots;
        EXPECT_TRUE(player->GetInventory().AddItem(&addedSlots, templateId, count));
        return addedSlots.empty() ? Protocol::Slot() : addedSlots[0];
    }

    int32 CountIn(const Protocol::Slot& slot)
    {
        Protocol::Slot* current = player->GetInventory().GetSlot(slot.type(), slot.slot_id());
        return (current != nullptr && current->has_item()) ? current->item().count() : 0;
    }

    static map<Protocol::StatType, int64> StatsOf(const optional<UseItemResult>& result)
    {
        map<Protocol::StatType, int64> stats;
        for (const Protocol::Stat& stat : result->updatedStats)
            stats[stat.type()] = stat.value();
        return stats;
    }

    /** 주입한 표. 테스트가 행을 빼고 다시 설치할 수 있다. */
    GamedataTables tables;
    PlayerRef player;
};

TEST_F(PlayerUseItemTest, HpPotionRestoresHpOnly)
{
    Protocol::Slot slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 3);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 400);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MP), 100);
    EXPECT_EQ(CountIn(slot), 2);

    map<Protocol::StatType, int64> expected = { {Protocol::STAT_TYPE_HP, 400} };
    EXPECT_EQ(StatsOf(result), expected) << "회복률이 0인 MP는 싣지 않는다";
}

TEST_F(PlayerUseItemTest, RestoreIsCappedAtMax)
{
    player->SetStatValue(Protocol::STAT_TYPE_HP, 900);
    Protocol::Slot slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 1);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), MAX_HP);
}

TEST_F(PlayerUseItemTest, MixPotionRestoresBoth)
{
    Protocol::Slot slot = AddAndGetSlot(MIX_POTION_TEMPLATE_ID, 1);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    ASSERT_TRUE(result.has_value());

    map<Protocol::StatType, int64> expected = { {Protocol::STAT_TYPE_HP, 500}, {Protocol::STAT_TYPE_MP, 300} };
    EXPECT_EQ(StatsOf(result), expected);
    EXPECT_EQ(CountIn(slot), 0) << "마지막 한 개를 쓰면 슬롯이 빈다";
}

// 요청의 아이템은 클라이언트 슬롯이 어긋났는지 대조하는 데만 쓴다. 다르면 쓰지 않는다.
TEST_F(PlayerUseItemTest, RequestNotMatchingSlotIsRejected)
{
    Protocol::Slot slot = AddAndGetSlot(MP_POTION_TEMPLATE_ID, 1);
    slot.mutable_item()->set_template_id(MIX_POTION_TEMPLATE_ID); // 요청의 템플릿을 속인다

    auto result = player->ProcessUseItem(slot, NOW_MS);
    EXPECT_FALSE(result.has_value());

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MP), 100);
    EXPECT_EQ(CountIn(slot), 1) << "어긋난 요청으로 엉뚱한 물약을 소비하면 안 된다";
}

TEST_F(PlayerUseItemTest, FullStatsStillConsume)
{
    player->SetStatValue(Protocol::STAT_TYPE_HP, MAX_HP);
    Protocol::Slot slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 2);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), MAX_HP);
    EXPECT_EQ(CountIn(slot), 1);
}

TEST_F(PlayerUseItemTest, GearSlotIsRejected)
{
    Protocol::Slot slot = AddAndGetSlot(SWORD_TEMPLATE_ID, 1);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    EXPECT_FALSE(result.has_value());

    EXPECT_EQ(CountIn(slot), 1) << "소모품이 아닌 아이템이 효과 없이 사라지면 안 된다";
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
}

TEST_F(PlayerUseItemTest, EmptySlotIsRejected)
{
    Protocol::Slot slot;
    slot.set_type(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE);
    slot.set_slot_id(0);
    slot.mutable_item()->set_template_id(HP_POTION_TEMPLATE_ID);
    slot.mutable_item()->set_count(1);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
}

TEST_F(PlayerUseItemTest, UnknownTemplateIsRejected)
{
    Protocol::Slot slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 1);
    // 데이터에서 빠진 아이템이 인벤토리에 남아 있다
    tables.items.erase(HP_POTION_TEMPLATE_ID);
    Gamedata::Install(tables);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(CountIn(slot), 1);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
}

TEST_F(PlayerUseItemTest, DeadPlayerIsRejected)
{
    Protocol::Slot slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 1);
    player->OnDie(nullptr);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(CountIn(slot), 1);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
}

TEST_F(PlayerUseItemTest, SameTemplateWaitsForCooldown)
{
    Protocol::Slot slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 3);
    auto first = player->ProcessUseItem(slot, NOW_MS);
    ASSERT_TRUE(first.has_value());

    const uint64 cooldownMs = COOLDOWN_SECONDS * 1000ull;

    auto tooEarly = player->ProcessUseItem(slot, NOW_MS + cooldownMs - 1);
    const int64 hpAfterFirst = player->GetStatValue(Protocol::STAT_TYPE_HP);
    EXPECT_FALSE(tooEarly.has_value());
    EXPECT_EQ(CountIn(slot), 2) << "재사용 대기 중에 거부하면 개수가 그대로다";
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), hpAfterFirst);

    auto afterCooldown = player->ProcessUseItem(slot, NOW_MS + cooldownMs);
    EXPECT_TRUE(afterCooldown.has_value());
    EXPECT_EQ(CountIn(slot), 1);
}

TEST_F(PlayerUseItemTest, CooldownIsPerTemplate)
{
    Protocol::Slot hpSlot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 1);
    Protocol::Slot mpSlot = AddAndGetSlot(MP_POTION_TEMPLATE_ID, 1);

    auto first = player->ProcessUseItem(hpSlot, NOW_MS);
    ASSERT_TRUE(first.has_value());

    auto second = player->ProcessUseItem(mpSlot, NOW_MS);
    EXPECT_TRUE(second.has_value());
}
