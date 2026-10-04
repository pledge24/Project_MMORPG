#include "pch.h"
#include <gtest/gtest.h>
#include "Player.h"
#include "Inventory.h"

/*--------------------------------------------------------------
    소모품 사용 테스트

    소모품 가방에 든 아이템만 쓸 수 있다. 회복량은 요청이 아니라 슬롯에 든 아이템으로 정하고,
    요청의 아이템이 슬롯과 다르면 쓰지 않는다.
    HP나 MP가 가득 차 있어도 소비한다. 재사용 대기는 템플릿마다 따로 돌고 서버가 판정한다.

    픽스처 결합도: Gamedata::s_itemDataTable을 손으로 시드하고 Player를 Init()만 한다.
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

    Json MakePotion(double hpRestore, double mpRestore)
    {
        Json potion;
        potion[string(JsonProperty::Item::ItemType)] = "CONSUMABLE";
        potion[string(JsonProperty::Item::ItemSubtype)] = "potion";
        potion[string(JsonProperty::Item::MaxStack)] = 10;
        potion[string(JsonProperty::Item::Cooldown)] = COOLDOWN_SECONDS;
        potion[string(JsonProperty::Item::HpRestore)] = hpRestore;
        potion[string(JsonProperty::Item::MpRestore)] = mpRestore;
        return potion;
    }
}

class PlayerUseItemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Gamedata::s_itemDataTable[HP_POTION_TEMPLATE_ID] = MakePotion(0.3, 0);
        Gamedata::s_itemDataTable[MP_POTION_TEMPLATE_ID] = MakePotion(0, 0.3);
        Gamedata::s_itemDataTable[MIX_POTION_TEMPLATE_ID] = MakePotion(0.4, 0.4);

        Json sword;
        sword[string(JsonProperty::Item::ItemType)] = "GEAR";
        sword[string(JsonProperty::Item::ItemSubtype)] = string(JsonProperty::Item::GearSubtype_Sword);
        sword[string(JsonProperty::Item::MaxStack)] = 1;
        sword[string(JsonProperty::Item::HpRestore)] = 1.0;
        Gamedata::s_itemDataTable[SWORD_TEMPLATE_ID] = sword;

        player = make_shared<Player>();
        ASSERT_TRUE(player->Init());
        player->SetStatValue(Protocol::STAT_TYPE_MAX_HP, MAX_HP);
        player->SetStatValue(Protocol::STAT_TYPE_HP, 100);
        player->SetStatValue(Protocol::STAT_TYPE_MAX_MP, MAX_MP);
        player->SetStatValue(Protocol::STAT_TYPE_MP, 100);
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::s_itemDataTable.clear();
    }

    // 아이템을 넣고 그 슬롯을 요청 형태로 돌려준다.
    Protocol::Slot AddAndGetSlot(int32 templateId, int32 count)
    {
        RepeatedPtrField<Protocol::Slot> addedSlots;
        EXPECT_TRUE(player->_inventory->AddItem(&addedSlots, templateId, count));
        return addedSlots.empty() ? Protocol::Slot() : addedSlots[0];
    }

    int32 CountIn(const Protocol::Slot& slot)
    {
        Protocol::Slot* current = player->_inventory->GetSlot(slot.type(), slot.slot_id());
        return (current != nullptr && current->has_item()) ? current->item().count() : 0;
    }

    static map<Protocol::StatType, int64> StatsOf(const Protocol::S_USE_ITEM& pkt)
    {
        map<Protocol::StatType, int64> stats;
        for (const Protocol::Stat& stat : pkt.updated_stat())
            stats[stat.type()] = stat.value();
        return stats;
    }

    PlayerRef player;
};

TEST_F(PlayerUseItemTest, HpPotionRestoresHpOnly)
{
    Protocol::Slot slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 3);

    Protocol::S_USE_ITEM pkt;
    ASSERT_TRUE(player->ProcessUseItem(slot, NOW_MS, pkt));

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 400);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MP), 100);
    EXPECT_EQ(CountIn(slot), 2);

    map<Protocol::StatType, int64> expected = { {Protocol::STAT_TYPE_HP, 400} };
    EXPECT_EQ(StatsOf(pkt), expected) << "회복률이 0인 MP는 싣지 않는다";
}

TEST_F(PlayerUseItemTest, RestoreIsCappedAtMax)
{
    player->SetStatValue(Protocol::STAT_TYPE_HP, 900);
    Protocol::Slot slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 1);

    Protocol::S_USE_ITEM pkt;
    ASSERT_TRUE(player->ProcessUseItem(slot, NOW_MS, pkt));

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), MAX_HP);
}

TEST_F(PlayerUseItemTest, MixPotionRestoresBoth)
{
    Protocol::Slot slot = AddAndGetSlot(MIX_POTION_TEMPLATE_ID, 1);

    Protocol::S_USE_ITEM pkt;
    ASSERT_TRUE(player->ProcessUseItem(slot, NOW_MS, pkt));

    map<Protocol::StatType, int64> expected = { {Protocol::STAT_TYPE_HP, 500}, {Protocol::STAT_TYPE_MP, 300} };
    EXPECT_EQ(StatsOf(pkt), expected);
    EXPECT_EQ(CountIn(slot), 0) << "마지막 한 개를 쓰면 슬롯이 빈다";
}

// 요청의 아이템은 클라이언트 슬롯이 어긋났는지 대조하는 데만 쓴다. 다르면 쓰지 않는다.
TEST_F(PlayerUseItemTest, RequestNotMatchingSlotIsRejected)
{
    Protocol::Slot slot = AddAndGetSlot(MP_POTION_TEMPLATE_ID, 1);
    slot.mutable_item()->set_template_id(MIX_POTION_TEMPLATE_ID); // 요청의 템플릿을 속인다

    Protocol::S_USE_ITEM pkt;
    EXPECT_FALSE(player->ProcessUseItem(slot, NOW_MS, pkt));

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MP), 100);
    EXPECT_EQ(CountIn(slot), 1) << "어긋난 요청으로 엉뚱한 물약을 소비하면 안 된다";
}

TEST_F(PlayerUseItemTest, FullStatsStillConsume)
{
    player->SetStatValue(Protocol::STAT_TYPE_HP, MAX_HP);
    Protocol::Slot slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 2);

    Protocol::S_USE_ITEM pkt;
    ASSERT_TRUE(player->ProcessUseItem(slot, NOW_MS, pkt));

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), MAX_HP);
    EXPECT_EQ(CountIn(slot), 1);
}

TEST_F(PlayerUseItemTest, GearSlotIsRejected)
{
    Protocol::Slot slot = AddAndGetSlot(SWORD_TEMPLATE_ID, 1);

    Protocol::S_USE_ITEM pkt;
    EXPECT_FALSE(player->ProcessUseItem(slot, NOW_MS, pkt));

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

    Protocol::S_USE_ITEM pkt;
    EXPECT_FALSE(player->ProcessUseItem(slot, NOW_MS, pkt));
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
    EXPECT_EQ(pkt.updated_slots_size(), 0);
}

TEST_F(PlayerUseItemTest, UnknownTemplateIsRejected)
{
    Protocol::Slot slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 1);
    Gamedata::s_itemDataTable.erase(HP_POTION_TEMPLATE_ID); // 데이터에서 빠진 아이템이 인벤토리에 남아 있다

    Protocol::S_USE_ITEM pkt;
    EXPECT_FALSE(player->ProcessUseItem(slot, NOW_MS, pkt));
    EXPECT_EQ(CountIn(slot), 1);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
}

TEST_F(PlayerUseItemTest, DeadPlayerIsRejected)
{
    Protocol::Slot slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 1);
    player->OnDie(nullptr);

    Protocol::S_USE_ITEM pkt;
    EXPECT_FALSE(player->ProcessUseItem(slot, NOW_MS, pkt));
    EXPECT_EQ(CountIn(slot), 1);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), 100);
}

TEST_F(PlayerUseItemTest, SameTemplateWaitsForCooldown)
{
    Protocol::Slot slot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 3);
    Protocol::S_USE_ITEM first;
    ASSERT_TRUE(player->ProcessUseItem(slot, NOW_MS, first));

    const uint64 cooldownMs = COOLDOWN_SECONDS * 1000ull;

    Protocol::S_USE_ITEM tooEarly;
    const int64 hpAfterFirst = player->GetStatValue(Protocol::STAT_TYPE_HP);
    EXPECT_FALSE(player->ProcessUseItem(slot, NOW_MS + cooldownMs - 1, tooEarly));
    EXPECT_EQ(CountIn(slot), 2) << "재사용 대기 중에 거부하면 개수가 그대로다";
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), hpAfterFirst);
    EXPECT_EQ(tooEarly.updated_slots_size(), 0) << "거부 응답에 슬롯을 싣지 않는다";

    Protocol::S_USE_ITEM afterCooldown;
    EXPECT_TRUE(player->ProcessUseItem(slot, NOW_MS + cooldownMs, afterCooldown));
    EXPECT_EQ(CountIn(slot), 1);
}

TEST_F(PlayerUseItemTest, CooldownIsPerTemplate)
{
    Protocol::Slot hpSlot = AddAndGetSlot(HP_POTION_TEMPLATE_ID, 1);
    Protocol::Slot mpSlot = AddAndGetSlot(MP_POTION_TEMPLATE_ID, 1);

    Protocol::S_USE_ITEM first;
    ASSERT_TRUE(player->ProcessUseItem(hpSlot, NOW_MS, first));

    Protocol::S_USE_ITEM second;
    EXPECT_TRUE(player->ProcessUseItem(mpSlot, NOW_MS, second));
}
