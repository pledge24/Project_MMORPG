#include "Core/pch.h"
#include <gtest/gtest.h>
#include "DB/ItemRows.h"

/*--------------------------------------------------------------
    DB 아이템 행과 패킷 칸의 변환 테스트

    DB는 아이템의 자리를 컨테이너(인벤토리, 착용 장비), 구역(아이템 종류), 구역 안의 칸 번호로 적고,
    패킷은 칸 종류(SlotType)와 slot_id로 적는다. 불러오기와 저장이 같은 변환을 써야, 저장한 자리에서
    다시 불러온다. 한쪽만 어긋나면 아이템이 다른 구역이나 다른 부위로 옮겨 가거나 사라진다.

    픽스처 결합도: 행과 Possession을 손으로 만든다. DB와 기획 데이터는 쓰지 않는다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 SWORD_UID = 1001;
    constexpr int32 SWORD_TEMPLATE_ID = 10;
    constexpr int64 POTION_UID = 2001;
    constexpr int32 POTION_TEMPLATE_ID = 20;

    ItemRow MakeRow(ItemContainer container, Protocol::ItemType itemType, int32 slotIndex, int64 itemUid = POTION_UID, int32 templateId = POTION_TEMPLATE_ID)
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

    ItemRow MakeGearRow(ItemContainer container, int32 slotIndex)
    {
        ItemRow row = MakeRow(container, Protocol::ITEM_TYPE_GEAR, slotIndex, SWORD_UID, SWORD_TEMPLATE_ID);
        row.enhanceLevel = 3;
        row.durability = 0;
        return row;
    }

    Protocol::Slot MakeSlot(Protocol::SlotType slotType, int32 slotId, int64 itemUid, int32 templateId, int32 count)
    {
        Protocol::Slot slot;
        slot.set_type(slotType);
        slot.set_slot_id(slotId);
        Protocol::Item* item = slot.mutable_item();
        item->set_item_uid(itemUid);
        item->set_template_id(templateId);
        item->set_count(count);
        return slot;
    }

    // 컨테이너와 구역마다 행 하나씩. 불러오기와 저장의 왕복에 쓴다.
    vector<ItemRow> MakeRowsOfEveryPlace()
    {
        ItemRow inventoryGear = MakeGearRow(ItemContainer::INVENTORY, 4);
        ItemRow consumable = MakeRow(ItemContainer::INVENTORY, Protocol::ITEM_TYPE_CONSUMABLE, 0, POTION_UID, POTION_TEMPLATE_ID);
        consumable.stackCount = 7;
        ItemRow misc = MakeRow(ItemContainer::INVENTORY, Protocol::ITEM_TYPE_MISCELLANEOUS, 2, 3001, 30);
        ItemRow equipped = MakeGearRow(ItemContainer::EQUIPPED, Protocol::GEAR_TYPE_WEAPON);
        equipped.itemUid = 1002;
        equipped.enhanceLevel = 0;

        return { inventoryGear, consumable, misc, equipped };
    }
}

/* 불러오기: 행 → 패킷 칸 */

TEST(ItemRowsTest, InventoryRowGoesToSlotListOfItsSection)
{
    struct Case
    {
        Protocol::ItemType section;
        Protocol::SlotType slotType;
        const RepeatedPtrField<Protocol::Slot>& (Protocol::Inventory::*slots)() const;
    };
    const Case cases[] = {
        { Protocol::ITEM_TYPE_GEAR, Protocol::SLOT_TYPE_INVENTORY_GEAR, &Protocol::Inventory::gear },
        { Protocol::ITEM_TYPE_CONSUMABLE, Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, &Protocol::Inventory::consumables },
        { Protocol::ITEM_TYPE_MISCELLANEOUS, Protocol::SLOT_TYPE_INVENTORY_MISC, &Protocol::Inventory::miscellaneous },
    };

    for (const Case& testCase : cases)
    {
        Protocol::Possession possession;
        ASSERT_TRUE(ItemRows::AddToPossession(MakeRow(ItemContainer::INVENTORY, testCase.section, 9), OUT possession));

        const RepeatedPtrField<Protocol::Slot>& slots = (possession.inventory().*testCase.slots)();
        ASSERT_EQ(slots.size(), 1) << "구역 " << testCase.section;
        EXPECT_EQ(slots[0].type(), testCase.slotType);
        EXPECT_EQ(slots[0].slot_id(), 9) << "칸 번호는 구역 안의 번호 그대로다";
        EXPECT_EQ(possession.inventory().gear().size() + possession.inventory().consumables().size()
            + possession.inventory().miscellaneous().size(), 1) << "다른 구역에는 붙이지 않는다";
    }
}

TEST(ItemRowsTest, EquippedRowGoesToItsGearPart)
{
    Protocol::Possession possession;
    ASSERT_TRUE(ItemRows::AddToPossession(MakeGearRow(ItemContainer::EQUIPPED, Protocol::GEAR_TYPE_WEAPON), OUT possession));

    ASSERT_EQ(possession.equipped_gear().count(Protocol::GEAR_TYPE_WEAPON), 1u) << "착용 장비의 칸 번호는 장비 부위다";
    const Protocol::Slot& slot = possession.equipped_gear().at(Protocol::GEAR_TYPE_WEAPON);
    EXPECT_EQ(slot.type(), Protocol::SLOT_TYPE_EQUIPPED);
    EXPECT_EQ(slot.slot_id(), Protocol::GEAR_TYPE_WEAPON);
    EXPECT_EQ(possession.inventory().gear().size(), 0);
}

TEST(ItemRowsTest, RowValuesBecomeItem)
{
    ItemRow row = MakeRow(ItemContainer::INVENTORY, Protocol::ITEM_TYPE_CONSUMABLE, 0);
    row.stackCount = 7;

    Protocol::Possession possession;
    ASSERT_TRUE(ItemRows::AddToPossession(row, OUT possession));

    const Protocol::Item& item = possession.inventory().consumables(0).item();
    EXPECT_EQ(item.item_uid(), POTION_UID);
    EXPECT_EQ(item.template_id(), POTION_TEMPLATE_ID);
    EXPECT_EQ(item.count(), 7);
    EXPECT_FALSE(item.has_gearinfo()) << "장비가 아닌 아이템에는 장비 정보가 없다";
}

TEST(ItemRowsTest, GearRowCarriesGearInfoWithZeroAdditionalAttack)
{
    Protocol::Possession possession;
    ASSERT_TRUE(ItemRows::AddToPossession(MakeGearRow(ItemContainer::INVENTORY, 0), OUT possession));

    const Protocol::Item& item = possession.inventory().gear(0).item();
    ASSERT_TRUE(item.has_gearinfo());
    EXPECT_EQ(item.gearinfo().enhance_level(), 3);
    EXPECT_EQ(item.gearinfo().durability(), 0);
    EXPECT_EQ(item.gearinfo().additional_physical_attack(), 0) << "추가 공격력은 DB에 없으므로 0을 채운다";
    EXPECT_EQ(item.gearinfo().additional_magical_attack(), 0);
}

TEST(ItemRowsTest, UnknownPlaceIsRejected)
{
    ItemRow unknownContainer = MakeRow(static_cast<ItemContainer>(3), Protocol::ITEM_TYPE_CONSUMABLE, 0);
    ItemRow noSection = MakeRow(ItemContainer::INVENTORY, Protocol::ITEM_TYPE_NONE, 0);
    ItemRow unknownSection = MakeRow(ItemContainer::INVENTORY, static_cast<Protocol::ItemType>(9), 0);
    ItemRow equippedConsumable = MakeRow(ItemContainer::EQUIPPED, Protocol::ITEM_TYPE_CONSUMABLE, Protocol::GEAR_TYPE_WEAPON);

    for (const ItemRow& row : { unknownContainer, noSection, unknownSection, equippedConsumable })
    {
        Protocol::Possession possession;
        EXPECT_FALSE(ItemRows::AddToPossession(row, OUT possession))
            << "컨테이너 " << static_cast<int32>(row.container) << ", 구역 " << row.itemType;
        EXPECT_EQ(possession.ByteSizeLong(), 0u) << "거절한 행은 아무것도 붙이지 않는다";
    }
}

TEST(ItemRowsTest, SecondRowInSameGearPartIsRejected)
{
    Protocol::Possession possession;
    ASSERT_TRUE(ItemRows::AddToPossession(MakeGearRow(ItemContainer::EQUIPPED, Protocol::GEAR_TYPE_WEAPON), OUT possession));

    ItemRow second = MakeGearRow(ItemContainer::EQUIPPED, Protocol::GEAR_TYPE_WEAPON);
    second.itemUid = SWORD_UID + 1;

    EXPECT_FALSE(ItemRows::AddToPossession(second, OUT possession)) << "부위 하나에 두 장비를 겹치면 하나가 사라진다";
    EXPECT_EQ(possession.equipped_gear().at(Protocol::GEAR_TYPE_WEAPON).item().item_uid(), SWORD_UID);
}

/* 저장: 패킷 칸 → 행 */

TEST(ItemRowsTest, OnlySlotsWithItemBecomeRows)
{
    Protocol::Possession possession;
    Protocol::Inventory* inventory = possession.mutable_inventory();
    Protocol::Slot* emptySlot = inventory->add_consumables();
    emptySlot->set_type(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE);
    emptySlot->set_slot_id(0);
    *inventory->add_consumables() = MakeSlot(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, 1, POTION_UID, POTION_TEMPLATE_ID, 5);

    Protocol::Slot emptyPart;
    emptyPart.set_type(Protocol::SLOT_TYPE_EQUIPPED);
    emptyPart.set_slot_id(Protocol::GEAR_TYPE_HELMET);
    (*possession.mutable_equipped_gear())[Protocol::GEAR_TYPE_HELMET] = emptyPart;

    const vector<ItemRow> rows = ItemRows::FromPossession(possession);

    ASSERT_EQ(rows.size(), 1u) << "빈 칸은 행이 되지 않는다. 그 칸에 있던 DB 행은 저장 쿼리가 지운다";
    EXPECT_EQ(rows[0].itemUid, POTION_UID);
    EXPECT_EQ(rows[0].container, ItemContainer::INVENTORY);
    EXPECT_EQ(rows[0].itemType, Protocol::ITEM_TYPE_CONSUMABLE);
    EXPECT_EQ(rows[0].slotIndex, 1);
    EXPECT_EQ(rows[0].stackCount, 5);
}

TEST(ItemRowsTest, InventorySlotBecomesRowOfItsSection)
{
    Protocol::Possession possession;
    Protocol::Inventory* inventory = possession.mutable_inventory();
    *inventory->add_gear() = MakeSlot(Protocol::SLOT_TYPE_INVENTORY_GEAR, 3, SWORD_UID, SWORD_TEMPLATE_ID, 1);
    *inventory->add_miscellaneous() = MakeSlot(Protocol::SLOT_TYPE_INVENTORY_MISC, 3, 3001, 30, 2);

    const vector<ItemRow> rows = ItemRows::FromPossession(possession);

    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].itemType, Protocol::ITEM_TYPE_GEAR);
    EXPECT_EQ(rows[1].itemType, Protocol::ITEM_TYPE_MISCELLANEOUS) << "같은 칸 번호여도 구역이 다르면 다른 칸이다";
    EXPECT_EQ(rows[0].slotIndex, 3);
    EXPECT_EQ(rows[1].slotIndex, 3);
}

TEST(ItemRowsTest, EquippedSlotBecomesGearRowAtItsPart)
{
    Protocol::Possession possession;
    Protocol::Slot slot = MakeSlot(Protocol::SLOT_TYPE_EQUIPPED, Protocol::GEAR_TYPE_WEAPON, SWORD_UID, SWORD_TEMPLATE_ID, 1);
    slot.mutable_item()->mutable_gearinfo()->set_enhance_level(5);
    slot.mutable_item()->mutable_gearinfo()->set_additional_physical_attack(9);
    (*possession.mutable_equipped_gear())[Protocol::GEAR_TYPE_WEAPON] = slot;

    const vector<ItemRow> rows = ItemRows::FromPossession(possession);

    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].container, ItemContainer::EQUIPPED);
    EXPECT_EQ(rows[0].itemType, Protocol::ITEM_TYPE_GEAR);
    EXPECT_EQ(rows[0].slotIndex, Protocol::GEAR_TYPE_WEAPON);
    EXPECT_EQ(rows[0].enhanceLevel, 5);
}

TEST(ItemRowsTest, RowsSurviveRoundTrip)
{
    const vector<ItemRow> rows = MakeRowsOfEveryPlace();

    Protocol::Possession possession;
    for (const ItemRow& row : rows)
        ASSERT_TRUE(ItemRows::AddToPossession(row, OUT possession));

    const vector<ItemRow> savedRows = ItemRows::FromPossession(possession);

    ASSERT_EQ(savedRows.size(), rows.size());
    for (const ItemRow& row : rows)
    {
        auto saved = ranges::find(savedRows, row.itemUid, &ItemRow::itemUid);
        ASSERT_NE(saved, savedRows.end()) << "아이템 " << row.itemUid << "이 저장 행에서 빠졌다";
        EXPECT_EQ(saved->templateId, row.templateId);
        EXPECT_EQ(saved->itemType, row.itemType);
        EXPECT_EQ(saved->container, row.container);
        EXPECT_EQ(saved->slotIndex, row.slotIndex);
        EXPECT_EQ(saved->stackCount, row.stackCount);
        EXPECT_EQ(saved->enhanceLevel, row.enhanceLevel);
        EXPECT_EQ(saved->durability, row.durability);
    }
}
