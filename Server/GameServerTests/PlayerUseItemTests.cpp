#include "Core/pch.h"
#include <gtest/gtest.h>
#include "PlayerTestAccess.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Inventory/InventoryComponent.h"

/*--------------------------------------------------------------
    소모품 사용 테스트

    소모품 가방에 든 아이템만 쓸 수 있다. 회복량은 요청이 아니라 칸에 든 아이템으로 정하고,
    요청의 uid가 칸의 아이템과 다르면 쓰지 않는다.
    HP나 MP가 가득 차 있어도 소비한다. 재사용 대기는 템플릿마다 따로 돌고 서버가 판정한다.

    픽스처 결합도: 아이템 템플릿과 인벤토리 구역 표를 Gamedata::Install로 주입하고 Player를 세션 없이 EntityFactory로만 만든다.
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
        sword.gearType = Protocol::GEAR_PART_WEAPON;
        sword.hpRestoreRatio = 1.0;
        tables.items[SWORD_TEMPLATE_ID] = sword;
        tables.inventorySlotCounts = MakeInventorySections();
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

    // 아이템을 넣고 그 칸을 요청 형태로 돌려준다.
    ItemSlotRequest AddAndGetSlot(int32 templateId, int32 count)
    {
        optional<SlotAddress> address = PlayerTestAccess::AddNewItem(*player, templateId, count);
        EXPECT_TRUE(address.has_value());
        return address.has_value() ? MakeInventoryRequest(*player, address.value()) : ItemSlotRequest();
    }

    int32 CountIn(const ItemSlotRequest& slot)
    {
        const ItemInstance* current = player->GetInventory().FindItem(slot.address);
        return current != nullptr ? current->count : 0;
    }

    static map<Protocol::StatType, int64> StatsOf(const ItemOutcome<UseItemResult>& result)
    {
        map<Protocol::StatType, int64> stats;
        for (const Protocol::Stat& stat : result->changedStats)
            stats[stat.type()] = stat.value();
        return stats;
    }

    /** 주입한 표. 테스트가 행을 빼고 다시 설치할 수 있다. */
    GamedataTables tables;
    PlayerRef player;
};

TEST_F(PlayerUseItemTest, HpPotionRestoresHpOnly)
{
    ItemSlotRequest slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 3);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    ASSERT_TRUE(result.IsSuccess());

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 400);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MP), 100);
    EXPECT_EQ(CountIn(slot), 2);

    map<Protocol::StatType, int64> expected = { {Protocol::STAT_TYPE_HP, 400} };
    EXPECT_EQ(StatsOf(result), expected) << "회복률이 0인 MP는 싣지 않는다";
}

TEST_F(PlayerUseItemTest, RestoreIsCappedAtMax)
{
    player->SetStatValue(Protocol::STAT_TYPE_HP, 900);
    ItemSlotRequest slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 1);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    ASSERT_TRUE(result.IsSuccess());

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), MAX_HP);
}

TEST_F(PlayerUseItemTest, MixPotionRestoresBoth)
{
    ItemSlotRequest slot = AddAndGetSlot(MIX_POTION_TEMPLATE_ID, 1);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    ASSERT_TRUE(result.IsSuccess());

    map<Protocol::StatType, int64> expected = { {Protocol::STAT_TYPE_HP, 500}, {Protocol::STAT_TYPE_MP, 300} };
    EXPECT_EQ(StatsOf(result), expected);
    EXPECT_EQ(CountIn(slot), 0) << "마지막 한 개를 쓰면 칸이 빈다";
}

// 요청의 uid는 클라이언트의 칸이 어긋났는지 대조하는 데만 쓴다. 다르면 쓰지 않는다.
TEST_F(PlayerUseItemTest, RequestNotMatchingSlotIsRejected)
{
    ItemSlotRequest slot = AddAndGetSlot(MP_POTION_TEMPLATE_ID, 1);
    slot.itemUid += 1; // 클라이언트는 그 칸에 다른 아이템이 있다고 본다

    auto result = player->ProcessUseItem(slot, NOW_MS);
    EXPECT_EQ(result.GetResult(), Protocol::ITEM_RESULT_SLOT_CHANGED);

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MP), 100);
    EXPECT_EQ(CountIn(slot), 1) << "어긋난 요청으로 엉뚱한 물약을 소비하면 안 된다";
}

TEST_F(PlayerUseItemTest, FullStatsStillConsume)
{
    player->SetStatValue(Protocol::STAT_TYPE_HP, MAX_HP);
    ItemSlotRequest slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 2);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    ASSERT_TRUE(result.IsSuccess());

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), MAX_HP);
    EXPECT_EQ(CountIn(slot), 1);
}

TEST_F(PlayerUseItemTest, GearSlotIsRejected)
{
    ItemSlotRequest slot = AddAndGetSlot(SWORD_TEMPLATE_ID, 1);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    EXPECT_EQ(result.GetResult(), Protocol::ITEM_RESULT_INVALID_REQUEST) << "정상 클라이언트는 소모품 구역이 아닌 칸의 사용을 보내지 않는다";

    EXPECT_EQ(CountIn(slot), 1) << "소모품이 아닌 아이템이 효과 없이 사라지면 안 된다";
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
}

TEST_F(PlayerUseItemTest, EmptySlotIsRejected)
{
    ItemSlotRequest slot;
    slot.address = SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 0 };
    slot.itemUid = 777;

    auto result = player->ProcessUseItem(slot, NOW_MS);
    EXPECT_EQ(result.GetResult(), Protocol::ITEM_RESULT_SLOT_CHANGED);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
}

TEST_F(PlayerUseItemTest, UnknownTemplateIsRejected)
{
    ItemSlotRequest slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 1);
    // 데이터에서 빠진 아이템이 인벤토리에 남아 있다
    tables.items.erase(HP_POTION_TEMPLATE_ID);
    Gamedata::Install(tables);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    EXPECT_EQ(result.GetResult(), Protocol::ITEM_RESULT_INTERNAL_ERROR);
    EXPECT_EQ(CountIn(slot), 1);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
}

TEST_F(PlayerUseItemTest, DeadPlayerIsRejected)
{
    ItemSlotRequest slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 1);
    player->OnDie(nullptr);

    auto result = player->ProcessUseItem(slot, NOW_MS);
    EXPECT_EQ(result.GetResult(), Protocol::ITEM_RESULT_DEAD);
    EXPECT_EQ(CountIn(slot), 1);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
}

TEST_F(PlayerUseItemTest, SameTemplateWaitsForCooldown)
{
    ItemSlotRequest slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 3);
    auto first = player->ProcessUseItem(slot, NOW_MS);
    ASSERT_TRUE(first.IsSuccess());

    const uint64 cooldownMs = COOLDOWN_SECONDS * 1000ull;

    auto tooEarly = player->ProcessUseItem(slot, NOW_MS + cooldownMs - 1);
    const int64 hpAfterFirst = player->GetStatValue(Protocol::STAT_TYPE_HP);
    EXPECT_EQ(tooEarly.GetResult(), Protocol::ITEM_RESULT_COOLDOWN);
    EXPECT_EQ(CountIn(slot), 2) << "재사용 대기 중에 거부하면 개수가 그대로다";
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), hpAfterFirst);

    auto afterCooldown = player->ProcessUseItem(slot, NOW_MS + cooldownMs);
    EXPECT_TRUE(afterCooldown.IsSuccess());
    EXPECT_EQ(CountIn(slot), 1);
}

TEST_F(PlayerUseItemTest, CooldownIsPerTemplate)
{
    ItemSlotRequest hpSlot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 1);
    ItemSlotRequest mpSlot = AddAndGetSlot(MP_POTION_TEMPLATE_ID, 1);

    auto first = player->ProcessUseItem(hpSlot, NOW_MS);
    ASSERT_TRUE(first.IsSuccess());

    auto second = player->ProcessUseItem(mpSlot, NOW_MS);
    EXPECT_TRUE(second.IsSuccess());
}
