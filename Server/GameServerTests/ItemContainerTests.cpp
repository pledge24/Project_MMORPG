#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Items/ItemContainer.h"

/*--------------------------------------------------------------
    범용 칸 배열

    넣기와 빼기, 클라이언트가 보낼 수 있는 범위 밖 주소, 0부터 세지 않는 칸 번호 집합,
    응답에 실을 바뀐 칸 목록을 본다. 기획 데이터와 플레이어 없이 돈다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 SLOT_COUNT = 4;

    ItemInstance MakeItem(int32 templateId, int32 count = 1, int64 itemUid = 100)
    {
        ItemInstance item;
        item.itemUid = itemUid;
        item.templateId = templateId;
        item.count = count;
        return item;
    }

    vector<SlotChange> TakeAll(ItemContainer& container)
    {
        vector<SlotChange> changes;
        container.TakeChanges(changes);
        return changes;
    }
}

TEST(ItemContainerTest, PutItemIsFoundInThatSlot)
{
    ItemContainer container(ContainerType::INVENTORY_CONSUMABLE, SLOT_COUNT);

    ASSERT_TRUE(container.Put(2, MakeItem(2001, 5)));

    const ItemInstance* stored = container.Find(2);
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(*stored, MakeItem(2001, 5));
    EXPECT_EQ(container.Find(1), nullptr) << "다른 칸이 찼다";
}

TEST(ItemContainerTest, PutIntoOccupiedSlotChangesNothing)
{
    ItemContainer container(ContainerType::INVENTORY_GEAR, SLOT_COUNT);
    ASSERT_TRUE(container.Put(0, MakeItem(1001, 1, 1)));

    EXPECT_FALSE(container.Put(0, MakeItem(1002, 1, 2)));
    EXPECT_EQ(container.Find(0)->templateId, 1001) << "찬 칸을 덮어썼다";
}

TEST(ItemContainerTest, PutWithCountBelowOneIsRejected)
{
    ItemContainer container(ContainerType::INVENTORY_MISC, SLOT_COUNT);

    EXPECT_FALSE(container.Put(0, MakeItem(3001, 0)));
    EXPECT_EQ(container.Find(0), nullptr);
}

TEST(ItemContainerTest, TakeEmptiesTheSlotAndReturnsItsItem)
{
    ItemContainer container(ContainerType::INVENTORY_GEAR, SLOT_COUNT);
    ASSERT_TRUE(container.Put(1, MakeItem(1001)));

    optional<ItemInstance> taken = container.Take(1);
    ASSERT_TRUE(taken.has_value());
    EXPECT_EQ(*taken, MakeItem(1001));
    EXPECT_EQ(container.Find(1), nullptr);

    EXPECT_FALSE(container.Take(1).has_value()) << "빈 칸에서 아이템이 나왔다";
}

TEST(ItemContainerTest, CountChangesKeepTheItemUntilItRunsOut)
{
    ItemContainer container(ContainerType::INVENTORY_CONSUMABLE, SLOT_COUNT);
    ASSERT_TRUE(container.Put(0, MakeItem(2001, 3)));

    ASSERT_TRUE(container.AddCount(0, 2));
    EXPECT_EQ(container.Find(0)->count, 5);

    EXPECT_FALSE(container.RemoveCount(0, 6)) << "든 개수보다 많이 뺐다";
    EXPECT_EQ(container.Find(0)->count, 5);

    ASSERT_TRUE(container.RemoveCount(0, 4));
    EXPECT_EQ(container.Find(0)->count, 1);

    ASSERT_TRUE(container.RemoveCount(0, 1));
    EXPECT_EQ(container.Find(0), nullptr) << "다 쓴 칸이 비지 않았다";
}

TEST(ItemContainerTest, CountChangesOnEmptySlotOrNonPositiveAmountAreRejected)
{
    ItemContainer container(ContainerType::INVENTORY_CONSUMABLE, SLOT_COUNT);
    EXPECT_FALSE(container.AddCount(0, 1));
    EXPECT_FALSE(container.RemoveCount(0, 1));

    ASSERT_TRUE(container.Put(0, MakeItem(2001, 3)));
    EXPECT_FALSE(container.AddCount(0, 0));
    EXPECT_FALSE(container.AddCount(0, -1));
    EXPECT_FALSE(container.RemoveCount(0, 0));
    EXPECT_FALSE(container.RemoveCount(0, -1));
    EXPECT_EQ(container.Find(0)->count, 3);
}

// 칸 번호는 클라이언트 값이 그대로 들어온다. 범위 밖이어도 정의된 실패로 끝나야 한다.
TEST(ItemContainerTest, OutOfRangeSlotIsRejectedEverywhere)
{
    ItemContainer container(ContainerType::INVENTORY_GEAR, SLOT_COUNT);

    for (int32 slotIndex : { -1, SLOT_COUNT, SLOT_COUNT + 1, INT32_MAX, INT32_MIN })
    {
        SCOPED_TRACE(slotIndex);
        EXPECT_FALSE(container.IsValidSlot(slotIndex));
        EXPECT_EQ(container.Find(slotIndex), nullptr);
        EXPECT_FALSE(container.Put(slotIndex, MakeItem(1001)));
        EXPECT_FALSE(container.Take(slotIndex).has_value());
        EXPECT_FALSE(container.AddCount(slotIndex, 1));
        EXPECT_FALSE(container.RemoveCount(slotIndex, 1));
    }

    EXPECT_TRUE(TakeAll(container).empty()) << "거절한 주소가 바뀐 칸으로 남았다";

    // 경계 안쪽은 그대로 동작해야 한다.
    EXPECT_TRUE(container.Put(SLOT_COUNT - 1, MakeItem(1001)));
}

// 착용 장비처럼 칸 번호가 0부터가 아닌 컨테이너. 집합에 없는 번호는 범위 안이어도 칸이 아니다.
TEST(ItemContainerTest, OnlyGivenSlotIndicesAreValid)
{
    ItemContainer container(ContainerType::EQUIPPED, vector<int32>{ 1, 2, 3, 4, 5, 6 });

    EXPECT_EQ(container.GetSlotIndices(), (vector<int32>{ 1, 2, 3, 4, 5, 6 }));
    EXPECT_FALSE(container.IsValidSlot(0)) << "0번 칸이 생겼다";
    EXPECT_FALSE(container.Put(0, MakeItem(1001)));
    EXPECT_FALSE(container.IsValidSlot(7));

    EXPECT_TRUE(container.Put(6, MakeItem(1001)));
    EXPECT_EQ(container.FindEmptySlot(), optional<int32>(1));
}

TEST(ItemContainerTest, FindEmptySlotSkipsOccupiedSlots)
{
    ItemContainer container(ContainerType::INVENTORY_GEAR, SLOT_COUNT);
    ASSERT_TRUE(container.Put(0, MakeItem(1001, 1, 1)));
    ASSERT_TRUE(container.Put(2, MakeItem(1001, 1, 2)));

    EXPECT_EQ(container.FindEmptySlot(), optional<int32>(1));
    EXPECT_EQ(container.FindEmptySlot(2), optional<int32>(3));

    ASSERT_TRUE(container.Put(1, MakeItem(1001, 1, 3)));
    ASSERT_TRUE(container.Put(3, MakeItem(1001, 1, 4)));
    EXPECT_FALSE(container.FindEmptySlot().has_value()) << "가득 찬 컨테이너에서 빈 칸을 찾았다";
}

TEST(ItemContainerTest, ChangesCarryCurrentContentInFirstChangedOrder)
{
    ItemContainer container(ContainerType::INVENTORY_CONSUMABLE, SLOT_COUNT);
    ASSERT_TRUE(container.Put(3, MakeItem(2001, 2, 1)));
    ASSERT_TRUE(container.Put(0, MakeItem(2002, 1, 2)));
    ASSERT_TRUE(container.AddCount(3, 1));
    ASSERT_TRUE(container.RemoveCount(0, 1));

    const vector<SlotChange> expected = {
        { { ContainerType::INVENTORY_CONSUMABLE, 3 }, MakeItem(2001, 3, 1) },
        { { ContainerType::INVENTORY_CONSUMABLE, 0 }, nullopt },
    };
    EXPECT_EQ(TakeAll(container), expected) << "한 칸이 두 번 실렸거나 중간 내용이 실렸다";

    EXPECT_TRUE(TakeAll(container).empty()) << "꺼낸 목록이 비지 않았다";
}

TEST(ItemContainerTest, RejectedChangesAreNotListed)
{
    ItemContainer container(ContainerType::INVENTORY_GEAR, SLOT_COUNT);
    ASSERT_TRUE(container.Put(0, MakeItem(1001)));
    TakeAll(container);

    EXPECT_FALSE(container.Put(0, MakeItem(1002)));
    EXPECT_FALSE(container.Take(1).has_value());
    EXPECT_FALSE(container.RemoveCount(0, 2));

    EXPECT_TRUE(TakeAll(container).empty());
}

TEST(ItemContainerTest, TakeChangesAppendsToGivenList)
{
    ItemContainer gear(ContainerType::INVENTORY_GEAR, SLOT_COUNT);
    ItemContainer equipped(ContainerType::EQUIPPED, vector<int32>{ 1, 2 });
    ASSERT_TRUE(equipped.Put(2, MakeItem(1001)));
    ASSERT_TRUE(gear.Take(0).has_value() == false);
    ASSERT_TRUE(gear.Put(1, MakeItem(1002)));

    vector<SlotChange> changes;
    equipped.TakeChanges(changes);
    gear.TakeChanges(changes);

    ASSERT_EQ(changes.size(), 2u);
    EXPECT_EQ(changes[0].address, (SlotAddress{ ContainerType::EQUIPPED, 2 }));
    EXPECT_EQ(changes[1].address, (SlotAddress{ ContainerType::INVENTORY_GEAR, 1 }));
}
