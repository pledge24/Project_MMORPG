#include "Core/pch.h"
#include <gtest/gtest.h>
#include "PlayerTestAccess.h"
#include "TestItemTables.h"
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
    constexpr int32 HP_POTION_TEMPLATE_ID = 2000;
    constexpr int32 MP_POTION_TEMPLATE_ID = 2001;
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
    constexpr int32 MISC_TEMPLATE_ID = 3000;
    constexpr int32 UNKNOWN_TEMPLATE_ID = 9999;
    constexpr int32 COOLDOWN_MS = 10'000;
    constexpr uint64 NOW_MS = 100'000;

    void AddPotion(GamedataTables& tables, int32 templateId)
    {
        TestItemTables::AddConsumable(tables, templateId, 99).CooldownMs = COOLDOWN_MS;
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

    /** 퀵슬롯 전체를 번호 순서로. registered에 없는 퀵슬롯은 비어 있다. */
    vector<QuickSlot> AllWith(initializer_list<QuickSlot> registered)
    {
        vector<QuickSlot> quickSlots;
        for (int32 slotIndex = 0; slotIndex < QUICK_SLOT_COUNT; slotIndex++)
            quickSlots.push_back(Empty(slotIndex));
        for (const QuickSlot& quickSlot : registered)
            quickSlots[quickSlot.slotIndex] = quickSlot;
        return quickSlots;
    }
}

class QuickSlotTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        AddPotion(tables, HP_POTION_TEMPLATE_ID);
        AddPotion(tables, MP_POTION_TEMPLATE_ID);
        TestItemTables::AddGear(tables, SWORD_TEMPLATE_ID, Protocol::GEAR_PART_WEAPON);
        TestItemTables::AddItem(tables, MISC_TEMPLATE_ID, Protocol::ITEM_TYPE_MISCELLANEOUS, 99);
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
        auto result = player->ProcessSetQuickSlot(slotIndex, ref);
        if (result.IsSuccess() == false)
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
    EXPECT_EQ(AllQuickSlots(), (AllWith({}))) << "퀵슬롯 개수는 서버 상수다";
}

TEST_F(QuickSlotTest, RegisteringConsumableChangesOnlyThatQuickSlot)
{
    EXPECT_EQ(Set(2, ItemRef(HP_POTION_TEMPLATE_ID)), (vector<QuickSlot>{ Registered(2, HP_POTION_TEMPLATE_ID) }));

    EXPECT_EQ(AllQuickSlots(), (AllWith({ Registered(2, HP_POTION_TEMPLATE_ID) })));
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
    EXPECT_EQ(AllQuickSlots(), (AllWith({})));
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
    EXPECT_EQ(AllQuickSlots(), (AllWith({ Registered(0, MP_POTION_TEMPLATE_ID), Registered(3, HP_POTION_TEMPLATE_ID) })));
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

    EXPECT_EQ(AllQuickSlots(), (AllWith({ Registered(0, HP_POTION_TEMPLATE_ID) }))) << "거절한 요청은 아무것도 바꾸지 않는다";
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

TEST_F(QuickSlotTest, RejectedRegistrationIsInvalidRequest)
{
    EXPECT_EQ(player->ProcessSetQuickSlot(QUICK_SLOT_COUNT, ItemRef(HP_POTION_TEMPLATE_ID)).GetResult(), Protocol::ITEM_RESULT_INVALID_REQUEST);
    EXPECT_EQ(player->ProcessSetQuickSlot(0, ItemRef(SWORD_TEMPLATE_ID)).GetResult(), Protocol::ITEM_RESULT_INVALID_REQUEST)
        << "정상 클라이언트는 소모품이 아닌 아이템을 퀵슬롯에 끌어다 놓지 못한다";
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
    EXPECT_FALSE(quickSlots.Load(Registered(QUICK_SLOT_COUNT, MP_POTION_TEMPLATE_ID))) << "퀵슬롯 개수를 줄이기 전에 저장한 행이다";
    EXPECT_FALSE(quickSlots.Load(Registered(1, SWORD_TEMPLATE_ID))) << "기획에서 아이템 종류를 바꿨다";
    EXPECT_FALSE(quickSlots.Load(Registered(2, MP_POTION_TEMPLATE_ID))) << "같은 퀵슬롯의 두 번째 행";
    EXPECT_FALSE(quickSlots.Load(Registered(0, HP_POTION_TEMPLATE_ID))) << "같은 참조는 퀵슬롯 하나에만 있다";
    EXPECT_FALSE(quickSlots.Load(Empty(0))) << "빈 퀵슬롯은 행이 없다";

    EXPECT_EQ(AllQuickSlots(), (AllWith({ Registered(2, HP_POTION_TEMPLATE_ID) })));
}

TEST_F(QuickSlotTest, SaveDataCarriesRegisteredQuickSlots)
{
    Register(3, MP_POTION_TEMPLATE_ID);

    const PlayerSaveData saveData = player->MakeSaveData();

    EXPECT_EQ(saveData.progress.quickSlots, (vector<QuickSlot>{ Registered(3, MP_POTION_TEMPLATE_ID) }));
}

/*--------------------------------------------------------------
    퀵슬롯 사용 테스트

    클라이언트는 퀵슬롯 번호만 보낸다. 서버가 그 퀵슬롯에 등록된 소모품이 든 칸 중 칸 번호가 가장 작은 칸을 골라
    소모품 사용으로 넘긴다. 그래서 사망과 재사용 대기 같은 사용 규칙은 칸을 골라 쓰는 소모품 사용과 같다.
    재사용 대기는 아이템 정의 단위이므로 같은 소모품의 다른 묶음도 함께 기다린다.

    픽스처 결합도: 등록 테스트의 픽스처에 더해, 아이템을 불러오기로 원하는 칸에 놓는다.
---------------------------------------------------------------*/

class QuickSlotUseTest : public QuickSlotTest
{
protected:
    /** 소모품 구역의 slotIndex 칸에 아이템을 놓는다. 묶음 쌓기를 거치지 않아 같은 아이템을 여러 칸에 나눠 둘 수 있다. */
    void PlaceConsumable(int32 slotIndex, int32 templateId, int32 count)
    {
        ItemInstance item;
        item.templateId = templateId;
        item.count = count;
        ASSERT_TRUE(PlayerTestAccess::Inventory(*player).LoadItem(ConsumableSlot(slotIndex), item));
        PlayerTestAccess::DiscardItemChanges(*player);
    }

    int32 CountAt(int32 slotIndex) const
    {
        const ItemInstance* item = player->GetInventory().FindItem(ConsumableSlot(slotIndex));
        return item != nullptr ? item->count : 0;
    }

    ItemOutcome<UseItemResult> Use(int32 quickSlotIndex, uint64 nowMs = NOW_MS)
    {
        return player->ProcessUseQuickSlot(quickSlotIndex, nowMs);
    }

    static SlotAddress ConsumableSlot(int32 slotIndex)
    {
        return SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, slotIndex };
    }
};

TEST_F(QuickSlotUseTest, UsesLowestSlotIndexAmongSplitStacks)
{
    PlaceConsumable(5, HP_POTION_TEMPLATE_ID, 3);
    PlaceConsumable(0, MP_POTION_TEMPLATE_ID, 1);
    PlaceConsumable(2, HP_POTION_TEMPLATE_ID, 2);
    Register(1, HP_POTION_TEMPLATE_ID);

    auto result = Use(1);
    ASSERT_TRUE(result.IsSuccess());

    EXPECT_EQ(CountAt(2), 1) << "같은 소모품이 여러 칸에 있으면 칸 번호가 가장 작은 칸을 쓴다";
    EXPECT_EQ(CountAt(5), 3);
    EXPECT_EQ(CountAt(0), 1) << "앞 칸에 든 다른 소모품은 쓰지 않는다";
    ASSERT_EQ(result->changedSlots.size(), 1u);
    EXPECT_EQ(result->changedSlots[0].address, ConsumableSlot(2)) << "응답은 칸을 골라 쓴 소모품 사용과 같은 칸 변경을 싣는다";
}

TEST_F(QuickSlotUseTest, ContinuesWithNextStackAfterFirstRunsOut)
{
    PlaceConsumable(2, HP_POTION_TEMPLATE_ID, 1);
    PlaceConsumable(5, HP_POTION_TEMPLATE_ID, 3);
    Register(0, HP_POTION_TEMPLATE_ID);

    ASSERT_TRUE(Use(0).IsSuccess());
    ASSERT_EQ(CountAt(2), 0) << "준비: 첫 묶음을 다 썼다";

    ASSERT_TRUE(Use(0, NOW_MS + COOLDOWN_MS).IsSuccess());

    EXPECT_EQ(CountAt(5), 2) << "다 쓴 뒤에는 남은 묶음 중 칸 번호가 가장 작은 칸으로 이어진다";
}

TEST_F(QuickSlotUseTest, UnregisteredQuickSlotIsRejected)
{
    PlaceConsumable(0, HP_POTION_TEMPLATE_ID, 1);
    Register(0, HP_POTION_TEMPLATE_ID);

    for (int32 quickSlotIndex : { 1, -1, QUICK_SLOT_COUNT })
    {
        SCOPED_TRACE(quickSlotIndex);
        EXPECT_EQ(Use(quickSlotIndex).GetResult(), Protocol::ITEM_RESULT_INVALID_REQUEST);
    }

    EXPECT_EQ(CountAt(0), 1) << "다른 퀵슬롯의 소모품을 쓰면 안 된다";
}

TEST_F(QuickSlotUseTest, RegisteredButNotOwnedIsRejected)
{
    PlaceConsumable(0, MP_POTION_TEMPLATE_ID, 1);
    Register(0, HP_POTION_TEMPLATE_ID);

    EXPECT_EQ(Use(0).GetResult(), Protocol::ITEM_RESULT_NOT_OWNED) << "다 떨어진 소모품도 등록은 남지만 쓸 것이 없다";
    EXPECT_EQ(CountAt(0), 1);
}

TEST_F(QuickSlotUseTest, CoolingDownIsRejectedAcrossStacks)
{
    PlaceConsumable(0, HP_POTION_TEMPLATE_ID, 1);
    PlaceConsumable(1, HP_POTION_TEMPLATE_ID, 2);
    Register(0, HP_POTION_TEMPLATE_ID);

    ASSERT_TRUE(Use(0).IsSuccess());

    EXPECT_EQ(Use(0, NOW_MS + COOLDOWN_MS - 1).GetResult(), Protocol::ITEM_RESULT_COOLDOWN) << "재사용 대기는 아이템 정의 단위라 다른 묶음도 기다린다";
    EXPECT_EQ(CountAt(1), 2);
}

TEST_F(QuickSlotUseTest, CooldownFromInventoryUseAppliesToQuickSlot)
{
    PlaceConsumable(0, HP_POTION_TEMPLATE_ID, 2);
    Register(0, HP_POTION_TEMPLATE_ID);
    ASSERT_TRUE(player->ProcessUseItem(MakeInventoryRequest(*player, ConsumableSlot(0)), NOW_MS).IsSuccess());

    EXPECT_EQ(Use(0, NOW_MS + 1).GetResult(), Protocol::ITEM_RESULT_COOLDOWN) << "인벤토리 창에서 쓴 소모품의 대기가 퀵슬롯에도 걸린다";
    EXPECT_EQ(CountAt(0), 1);
}

TEST_F(QuickSlotUseTest, DeadPlayerIsRejected)
{
    PlaceConsumable(0, HP_POTION_TEMPLATE_ID, 1);
    Register(0, HP_POTION_TEMPLATE_ID);
    player->OnDie(nullptr);

    EXPECT_EQ(Use(0).GetResult(), Protocol::ITEM_RESULT_DEAD);
    EXPECT_EQ(CountAt(0), 1);
}
