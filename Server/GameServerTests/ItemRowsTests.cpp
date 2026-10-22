#include "Core/pch.h"
#include <gtest/gtest.h>
#include "DB/ItemRows.h"
#include "Game/Items/InventorySections.h"

/*--------------------------------------------------------------
    DB 아이템 행과 칸에 든 아이템의 변환 테스트

    DB는 아이템의 자리를 컨테이너(인벤토리, 착용 장비), 구역(아이템 종류), 구역 안의 칸 번호로 적고,
    서버는 칸 주소(컨테이너 종류, 칸 번호)로 적는다. 불러오기와 저장이 같은 변환을 써야, 저장한 자리에서
    다시 불러온다. 한쪽만 어긋나면 아이템이 다른 구역이나 다른 부위로 옮겨 가거나 사라진다.

    픽스처 결합도: 행과 아이템을 손으로 만든다. DB와 기획 데이터는 쓰지 않는다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 SWORD_UID = 1001;
    constexpr int32 SWORD_TEMPLATE_ID = 10;
    constexpr int64 POTION_UID = 2001;
    constexpr int32 POTION_TEMPLATE_ID = 20;

    ItemRow MakeRow(ItemRowContainer container, ItemRowType itemType, int32 slotIndex, int64 itemUid = POTION_UID, int32 templateId = POTION_TEMPLATE_ID)
    {
        ItemRow row;
        row.itemUid = itemUid;
        row.templateId = templateId;
        row.itemType = itemType;
        row.container = container;
        row.slotIndex = slotIndex;
        row.stackCount = 1;
        return row;
    }

    ItemRow MakeGearRow(ItemRowContainer container, int32 slotIndex)
    {
        ItemRow row = MakeRow(container, ItemRowType::GEAR, slotIndex, SWORD_UID, SWORD_TEMPLATE_ID);
        row.enhanceLevel = 3;
        row.durability = 0;
        return row;
    }

    // 컨테이너와 구역마다 행 하나씩. 불러오기와 저장의 왕복에 쓴다.
    vector<ItemRow> MakeRowsOfEveryPlace()
    {
        ItemRow inventoryGear = MakeGearRow(ItemRowContainer::INVENTORY, 4);
        ItemRow consumable = MakeRow(ItemRowContainer::INVENTORY, ItemRowType::CONSUMABLE, 0, POTION_UID, POTION_TEMPLATE_ID);
        consumable.stackCount = 7;
        ItemRow misc = MakeRow(ItemRowContainer::INVENTORY, ItemRowType::MISC, 2, 3001, 30);
        ItemRow equipped = MakeGearRow(ItemRowContainer::EQUIPPED, Protocol::GEAR_TYPE_WEAPON);
        equipped.itemUid = 1002;
        equipped.enhanceLevel = 0;
        equipped.durability = 40;

        return { inventoryGear, consumable, misc, equipped };
    }
}

/* 불러오기: 행 → 칸에 든 아이템 */

TEST(ItemRowsTest, InventoryRowGoesToContainerOfItsSection)
{
    struct Case
    {
        ItemRowType section;
        ContainerType container;
    };
    const Case cases[] = {
        { ItemRowType::GEAR, ContainerType::INVENTORY_GEAR },
        { ItemRowType::CONSUMABLE, ContainerType::INVENTORY_CONSUMABLE },
        { ItemRowType::MISC, ContainerType::INVENTORY_MISC },
    };

    for (const Case& testCase : cases)
    {
        optional<PlacedItem> placed = ItemRows::ToPlacedItem(MakeRow(ItemRowContainer::INVENTORY, testCase.section, 9));
        ASSERT_TRUE(placed.has_value()) << "구역 " << static_cast<int32>(testCase.section);
        EXPECT_EQ(placed->address.container, testCase.container);
        EXPECT_EQ(placed->address.slotIndex, 9) << "칸 번호는 구역 안의 번호 그대로다";
    }
}

// DB의 ItemType 열은 아이템 종류의 번호를 그대로 담는다. 행의 구역이 게임의 구역 대응 표와 어긋나면
// 불러온 아이템이 게임이 판정하는 구역과 다른 구역에 들어간다.
TEST(ItemRowsTest, SectionOfRowTypeMatchesInventorySections)
{
    for (const InventorySection& section : InventorySections::ALL)
    {
        const ItemRowType rowType = static_cast<ItemRowType>(section.itemType);
        optional<PlacedItem> placed = ItemRows::ToPlacedItem(MakeRow(ItemRowContainer::INVENTORY, rowType, 0));
        ASSERT_TRUE(placed.has_value()) << "아이템 종류 " << section.itemType;
        EXPECT_EQ(placed->address.container, section.container);

        optional<ItemRow> row = ItemRows::ToRow(placed.value());
        ASSERT_TRUE(row.has_value());
        EXPECT_EQ(static_cast<int32>(row->itemType), static_cast<int32>(section.itemType));
    }
}

TEST(ItemRowsTest, EquippedRowGoesToItsGearPart)
{
    optional<PlacedItem> placed = ItemRows::ToPlacedItem(MakeGearRow(ItemRowContainer::EQUIPPED, Protocol::GEAR_TYPE_WEAPON));

    ASSERT_TRUE(placed.has_value());
    EXPECT_EQ(placed->address, (SlotAddress{ ContainerType::EQUIPPED, Protocol::GEAR_TYPE_WEAPON })) << "착용 장비의 칸 번호는 장비 부위다";
}

TEST(ItemRowsTest, RowValuesBecomeItem)
{
    ItemRow row = MakeRow(ItemRowContainer::INVENTORY, ItemRowType::CONSUMABLE, 0);
    row.stackCount = 7;

    optional<PlacedItem> placed = ItemRows::ToPlacedItem(row);
    ASSERT_TRUE(placed.has_value());

    EXPECT_EQ(placed->item.itemUid, POTION_UID);
    EXPECT_EQ(placed->item.templateId, POTION_TEMPLATE_ID);
    EXPECT_EQ(placed->item.count, 7);
    EXPECT_FALSE(placed->item.gear.has_value()) << "장비가 아닌 아이템에는 장비 세부 값이 없다";
}

TEST(ItemRowsTest, GearRowCarriesGearDetail)
{
    optional<PlacedItem> placed = ItemRows::ToPlacedItem(MakeGearRow(ItemRowContainer::INVENTORY, 0));

    ASSERT_TRUE(placed.has_value());
    ASSERT_TRUE(placed->item.gear.has_value());
    EXPECT_EQ(placed->item.gear->enhanceLevel, 3);
    EXPECT_EQ(placed->item.gear->durability, 0);
}

TEST(ItemRowsTest, UnknownPlaceIsRejected)
{
    ItemRow unknownContainer = MakeRow(static_cast<ItemRowContainer>(3), ItemRowType::CONSUMABLE, 0);
    ItemRow noSection = MakeRow(ItemRowContainer::INVENTORY, ItemRowType::NONE, 0);
    ItemRow unknownSection = MakeRow(ItemRowContainer::INVENTORY, static_cast<ItemRowType>(9), 0);
    ItemRow equippedConsumable = MakeRow(ItemRowContainer::EQUIPPED, ItemRowType::CONSUMABLE, Protocol::GEAR_TYPE_WEAPON);

    for (const ItemRow& row : { unknownContainer, noSection, unknownSection, equippedConsumable })
    {
        EXPECT_FALSE(ItemRows::ToPlacedItem(row).has_value())
            << "컨테이너 " << static_cast<int32>(row.container) << ", 구역 " << static_cast<int32>(row.itemType);
    }
}

/* 저장: 칸에 든 아이템 → 행 */

TEST(ItemRowsTest, InventoryItemBecomesRowOfItsSection)
{
    PlacedItem gear{ SlotAddress{ ContainerType::INVENTORY_GEAR, 3 }, ItemInstance{ SWORD_UID, SWORD_TEMPLATE_ID, 1, GearDetail{} } };
    PlacedItem misc{ SlotAddress{ ContainerType::INVENTORY_MISC, 3 }, ItemInstance{ 3001, 30, 2, nullopt } };

    optional<ItemRow> gearRow = ItemRows::ToRow(gear);
    optional<ItemRow> miscRow = ItemRows::ToRow(misc);

    ASSERT_TRUE(gearRow.has_value());
    ASSERT_TRUE(miscRow.has_value());
    EXPECT_EQ(gearRow->container, ItemRowContainer::INVENTORY);
    EXPECT_EQ(gearRow->itemType, ItemRowType::GEAR);
    EXPECT_EQ(miscRow->itemType, ItemRowType::MISC) << "같은 칸 번호여도 구역이 다르면 다른 칸이다";
    EXPECT_EQ(gearRow->slotIndex, 3);
    EXPECT_EQ(miscRow->slotIndex, 3);
    EXPECT_EQ(miscRow->stackCount, 2);
}

TEST(ItemRowsTest, EquippedItemBecomesGearRowAtItsPart)
{
    PlacedItem sword{ SlotAddress{ ContainerType::EQUIPPED, Protocol::GEAR_TYPE_WEAPON }, ItemInstance{ SWORD_UID, SWORD_TEMPLATE_ID, 1, GearDetail{ 5, 70 } } };

    optional<ItemRow> row = ItemRows::ToRow(sword);

    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->container, ItemRowContainer::EQUIPPED);
    EXPECT_EQ(row->itemType, ItemRowType::GEAR);
    EXPECT_EQ(row->slotIndex, Protocol::GEAR_TYPE_WEAPON);
    EXPECT_EQ(row->enhanceLevel, 5);
    EXPECT_EQ(row->durability, 70);
}

TEST(ItemRowsTest, AddressOutsideInventoryAndEquipmentIsRejected)
{
    PlacedItem nowhere{ SlotAddress{ ContainerType::NONE, 0 }, ItemInstance{ POTION_UID, POTION_TEMPLATE_ID, 1, nullopt } };

    EXPECT_FALSE(ItemRows::ToRow(nowhere).has_value());
}

TEST(ItemRowsTest, RowsSurviveRoundTrip)
{
    for (const ItemRow& row : MakeRowsOfEveryPlace())
    {
        SCOPED_TRACE(row.itemUid);

        optional<PlacedItem> placed = ItemRows::ToPlacedItem(row);
        ASSERT_TRUE(placed.has_value());
        optional<ItemRow> saved = ItemRows::ToRow(placed.value());
        ASSERT_TRUE(saved.has_value());

        EXPECT_EQ(saved->itemUid, row.itemUid);
        EXPECT_EQ(saved->templateId, row.templateId);
        EXPECT_EQ(saved->itemType, row.itemType);
        EXPECT_EQ(saved->container, row.container);
        EXPECT_EQ(saved->slotIndex, row.slotIndex);
        EXPECT_EQ(saved->stackCount, row.stackCount);
        EXPECT_EQ(saved->enhanceLevel, row.enhanceLevel);
        EXPECT_EQ(saved->durability, row.durability);
    }
}

TEST(ItemRowsTest, ItemsSurviveRoundTrip)
{
    const PlacedItem items[] = {
        { SlotAddress{ ContainerType::INVENTORY_GEAR, 0 }, ItemInstance{ SWORD_UID, SWORD_TEMPLATE_ID, 1, GearDetail{ 2, 50 } } },
        { SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 5 }, ItemInstance{ POTION_UID, POTION_TEMPLATE_ID, 9, nullopt } },
        { SlotAddress{ ContainerType::INVENTORY_MISC, 1 }, ItemInstance{ 3001, 30, 3, nullopt } },
        { SlotAddress{ ContainerType::EQUIPPED, Protocol::GEAR_TYPE_HELMET }, ItemInstance{ 1003, 11, 1, GearDetail{ 0, 10 } } },
    };

    for (const PlacedItem& item : items)
    {
        optional<ItemRow> row = ItemRows::ToRow(item);
        ASSERT_TRUE(row.has_value());
        EXPECT_EQ(ItemRows::ToPlacedItem(row.value()), optional<PlacedItem>(item));
    }
}
