#include "Core/pch.h"
#include <gtest/gtest.h>
#include "PlayerTestAccess.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/QuickSlots/QuickSlotComponent.h"

/*--------------------------------------------------------------
    퀵슬롯 등록 테스트

    퀵슬롯은 아이템을 담지 않고 소모품의 아이템 정의를 가리킨다. 등록 요청 하나(퀵슬롯 번호와 참조)로
    등록과 해제를 하고, 이미 다른 퀵슬롯에 있는 참조를 등록하면 두 퀵슬롯을 맞바꾼다. 서버는 소모품 종류인지만
    보고 소지 여부는 보지 않는다. 다 떨어진 소모품도 등록이 남아야 다시 샀을 때 그대로 쓴다.

    픽스처 결합도: 아이템 템플릿과 퀵슬롯 개수를 Gamedata::Install로 주입하고 Player를 세션 없이
    EntityFactory로만 만든다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 QUICK_SLOT_COUNT = 4;
    constexpr int32 HP_POTION_TEMPLATE_ID = 2000;
    constexpr int32 MP_POTION_TEMPLATE_ID = 2001;
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
    constexpr int32 MISC_TEMPLATE_ID = 3000;
    constexpr int32 UNKNOWN_TEMPLATE_ID = 9999;

    ItemTemplate MakeItem(int32 templateId, Protocol::ItemType itemType)
    {
        ItemTemplate item;
        item.templateId = templateId;
        item.itemType = itemType;
        item.maxStack = 99;
        if (itemType == Protocol::ITEM_TYPE_GEAR)
            item.gearType = Protocol::GEAR_TYPE_WEAPON;
        return item;
    }

    QuickSlotRef ItemRef(int32 templateId)
    {
        return QuickSlotRef{ QuickSlotRefType::ITEM, templateId };
    }

    QuickSlot Registered(int32 slotIndex, int32 templateId)
    {
        return QuickSlot{ slotIndex, ItemRef(templateId) };
    }

    QuickSlot Empty(int32 slotIndex)
    {
        return QuickSlot{ slotIndex, nullopt };
    }
}

class QuickSlotTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        tables.items[HP_POTION_TEMPLATE_ID] = MakeItem(HP_POTION_TEMPLATE_ID, Protocol::ITEM_TYPE_CONSUMABLE);
        tables.items[MP_POTION_TEMPLATE_ID] = MakeItem(MP_POTION_TEMPLATE_ID, Protocol::ITEM_TYPE_CONSUMABLE);
        tables.items[SWORD_TEMPLATE_ID] = MakeItem(SWORD_TEMPLATE_ID, Protocol::ITEM_TYPE_GEAR);
        tables.items[MISC_TEMPLATE_ID] = MakeItem(MISC_TEMPLATE_ID, Protocol::ITEM_TYPE_MISCELLANEOUS);
        tables.inventorySlotCounts = MakeInventorySections();
        tables.quickSlotCount = QUICK_SLOT_COUNT;
        Gamedata::Install(std::move(tables));

        player = EntityFactory::Create<Player>(PlayerSpawnParams());
        ASSERT_NE(player, nullptr);
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::Install(GamedataTables());
    }

    optional<vector<QuickSlot>> Set(int32 slotIndex, const optional<QuickSlotRef>& ref)
    {
        optional<SetQuickSlotResult> result = player->ProcessSetQuickSlot(slotIndex, ref);
        if (result.has_value() == false)
            return nullopt;

        return result->changedQuickSlots;
    }

    void Register(int32 slotIndex, int32 templateId)
    {
        ASSERT_TRUE(Set(slotIndex, ItemRef(templateId)).has_value());
    }

    /** 퀵슬롯 번호 순서로 모든 퀵슬롯의 지금 내용. */
    vector<QuickSlot> AllQuickSlots() const
    {
        return player->GetQuickSlots().GetAll();
    }

    PlayerRef player;
};

TEST_F(QuickSlotTest, EveryQuickSlotStartsEmpty)
{
    EXPECT_EQ(AllQuickSlots(), (vector<QuickSlot>{ Empty(0), Empty(1), Empty(2), Empty(3) })) << "퀵슬롯 개수는 기획 데이터의 값이다";
}

TEST_F(QuickSlotTest, RegisteringConsumableChangesOnlyThatQuickSlot)
{
    EXPECT_EQ(Set(2, ItemRef(HP_POTION_TEMPLATE_ID)), (vector<QuickSlot>{ Registered(2, HP_POTION_TEMPLATE_ID) }));

    EXPECT_EQ(AllQuickSlots(), (vector<QuickSlot>{ Empty(0), Empty(1), Registered(2, HP_POTION_TEMPLATE_ID), Empty(3) }));
}

TEST_F(QuickSlotTest, RegisteringDoesNotRequireOwningTheItem)
{
    ASSERT_EQ(player->GetInventory().FindSection(ContainerType::INVENTORY_CONSUMABLE)->Find(0), nullptr) << "준비: 소모품이 없다";

    EXPECT_TRUE(Set(0, ItemRef(HP_POTION_TEMPLATE_ID)).has_value()) << "다 떨어진 물약도 등록이 남아야 다시 사면 그대로 쓴다";
}

TEST_F(QuickSlotTest, ReplacingRegistrationOverwritesTheQuickSlot)
{
    Register(1, HP_POTION_TEMPLATE_ID);

    EXPECT_EQ(Set(1, ItemRef(MP_POTION_TEMPLATE_ID)), (vector<QuickSlot>{ Registered(1, MP_POTION_TEMPLATE_ID) }));
}

TEST_F(QuickSlotTest, EmptyRefClearsTheQuickSlot)
{
    Register(1, HP_POTION_TEMPLATE_ID);

    EXPECT_EQ(Set(1, nullopt), (vector<QuickSlot>{ Empty(1) }));
    EXPECT_EQ(AllQuickSlots(), (vector<QuickSlot>{ Empty(0), Empty(1), Empty(2), Empty(3) }));
}

TEST_F(QuickSlotTest, NoneTypeRefAlsoClears)
{
    Register(1, HP_POTION_TEMPLATE_ID);

    EXPECT_EQ(Set(1, QuickSlotRef{ QuickSlotRefType::NONE, 0 }), (vector<QuickSlot>{ Empty(1) }));
}

// 퀵슬롯끼리 끌어 놓으면 클라이언트는 놓은 자리에 끌어 온 참조를 등록한다. 같은 참조는 퀵슬롯 하나에만 있으므로 서로 바뀐다.
TEST_F(QuickSlotTest, RegisteringRefFromAnotherQuickSlotSwapsThem)
{
    Register(0, HP_POTION_TEMPLATE_ID);
    Register(3, MP_POTION_TEMPLATE_ID);

    EXPECT_EQ(Set(3, ItemRef(HP_POTION_TEMPLATE_ID)),
        (vector<QuickSlot>{ Registered(3, HP_POTION_TEMPLATE_ID), Registered(0, MP_POTION_TEMPLATE_ID) })) << "교환은 두 퀵슬롯이 바뀐다";
    EXPECT_EQ(AllQuickSlots(), (vector<QuickSlot>{ Registered(0, MP_POTION_TEMPLATE_ID), Empty(1), Empty(2), Registered(3, HP_POTION_TEMPLATE_ID) }));
}

TEST_F(QuickSlotTest, MovingRefToEmptyQuickSlotEmptiesTheOld)
{
    Register(0, HP_POTION_TEMPLATE_ID);

    EXPECT_EQ(Set(2, ItemRef(HP_POTION_TEMPLATE_ID)), (vector<QuickSlot>{ Registered(2, HP_POTION_TEMPLATE_ID), Empty(0) }));
}

TEST_F(QuickSlotTest, RegisteringSameRefInPlaceChangesNothing)
{
    Register(0, HP_POTION_TEMPLATE_ID);

    EXPECT_EQ(Set(0, ItemRef(HP_POTION_TEMPLATE_ID)), (vector<QuickSlot>{})) << "바뀐 퀵슬롯이 없어도 성공이다";
    EXPECT_EQ(Set(1, nullopt), (vector<QuickSlot>{})) << "빈 퀵슬롯의 해제도 성공이다";
}

TEST_F(QuickSlotTest, NonConsumableItemIsRejected)
{
    Register(0, HP_POTION_TEMPLATE_ID);

    for (int32 templateId : { SWORD_TEMPLATE_ID, MISC_TEMPLATE_ID, UNKNOWN_TEMPLATE_ID })
    {
        SCOPED_TRACE(templateId);
        EXPECT_EQ(Set(0, ItemRef(templateId)), nullopt);
    }

    EXPECT_EQ(AllQuickSlots(), (vector<QuickSlot>{ Registered(0, HP_POTION_TEMPLATE_ID), Empty(1), Empty(2), Empty(3) })) << "거절한 요청은 아무것도 바꾸지 않는다";
}

TEST_F(QuickSlotTest, UnknownRefTypeIsRejected)
{
    // 스킬을 등록하게 되면 2를 쓴다. 지금은 받지 않는다.
    EXPECT_EQ(Set(0, QuickSlotRef{ static_cast<QuickSlotRefType>(2), HP_POTION_TEMPLATE_ID }), nullopt);
}

TEST_F(QuickSlotTest, OutOfRangeQuickSlotIsRejected)
{
    for (int32 slotIndex : { -1, QUICK_SLOT_COUNT, 255 })
    {
        SCOPED_TRACE(slotIndex);
        EXPECT_EQ(Set(slotIndex, ItemRef(HP_POTION_TEMPLATE_ID)), nullopt);
        EXPECT_EQ(Set(slotIndex, nullopt), nullopt) << "해제도 범위 밖이면 거절한다";
    }
}

TEST_F(QuickSlotTest, CollectQuickSlotsSkipsEmptyOnes)
{
    Register(1, HP_POTION_TEMPLATE_ID);
    Register(3, MP_POTION_TEMPLATE_ID);

    vector<QuickSlot> collected;
    player->GetQuickSlots().CollectQuickSlots(OUT collected);

    EXPECT_EQ(collected, (vector<QuickSlot>{ Registered(1, HP_POTION_TEMPLATE_ID), Registered(3, MP_POTION_TEMPLATE_ID) })) << "빈 퀵슬롯은 DB에 행을 두지 않는다";
}

TEST_F(QuickSlotTest, LoadAcceptsOnlyValidRegistrations)
{
    QuickSlotComponent& quickSlots = PlayerTestAccess::QuickSlots(*player);

    EXPECT_TRUE(quickSlots.Load(Registered(2, HP_POTION_TEMPLATE_ID)));
    EXPECT_FALSE(quickSlots.Load(Registered(QUICK_SLOT_COUNT, MP_POTION_TEMPLATE_ID))) << "기획에서 퀵슬롯 개수를 줄였다";
    EXPECT_FALSE(quickSlots.Load(Registered(1, SWORD_TEMPLATE_ID))) << "기획에서 아이템 종류를 바꿨다";
    EXPECT_FALSE(quickSlots.Load(Registered(2, MP_POTION_TEMPLATE_ID))) << "같은 퀵슬롯의 두 번째 행";
    EXPECT_FALSE(quickSlots.Load(Registered(0, HP_POTION_TEMPLATE_ID))) << "같은 참조는 퀵슬롯 하나에만 있다";
    EXPECT_FALSE(quickSlots.Load(Empty(0))) << "빈 퀵슬롯은 행이 없다";

    EXPECT_EQ(AllQuickSlots(), (vector<QuickSlot>{ Empty(0), Empty(1), Registered(2, HP_POTION_TEMPLATE_ID), Empty(3) }));
}

TEST_F(QuickSlotTest, SaveDataCarriesRegisteredQuickSlots)
{
    Register(3, MP_POTION_TEMPLATE_ID);

    const PlayerSaveData saveData = player->MakeSaveData();

    EXPECT_EQ(saveData.progress.quickSlots, (vector<QuickSlot>{ Registered(3, MP_POTION_TEMPLATE_ID) }));
}
