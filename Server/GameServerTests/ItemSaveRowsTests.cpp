#include "Core/pch.h"
#include <gtest/gtest.h>
#include "DB/ItemSaveRows.h"

/*--------------------------------------------------------------
    저장할 아이템 행 고르기 테스트

    접속 종료 때 바뀐 슬롯만 DB에 반영한다. 행이 빠지면 그 변경은 재접속 뒤에 사라지고,
    비워진 슬롯이 행으로 나가지 않으면 버리거나 다 쓴 아이템이 되살아난다.

    픽스처 결합도: PlayerSaveData를 손으로 채운다. Player와 DB는 쓰지 않는다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 CHARACTER_ID = 77;
    constexpr int32 SLOT_COUNT = 3;

    Protocol::Slot MakeStackableSlot(int32 slotId, int32 templateId, int32 count)
    {
        Protocol::Slot slot;
        slot.set_slot_id(slotId);
        if (templateId > 0)
        {
            slot.mutable_item()->set_template_id(templateId);
            slot.mutable_item()->set_count(count);
        }
        return slot;
    }

    Protocol::Slot MakeGearSlot(int32 slotId, int32 templateId, int64 itemUid)
    {
        Protocol::Slot slot;
        slot.set_slot_id(slotId);
        Protocol::Item* item = slot.mutable_item();
        item->set_template_id(templateId);
        item->set_item_uid(itemUid);
        Protocol::GearInfo* gearInfo = item->mutable_gearinfo();
        gearInfo->set_enhance_level(3);
        gearInfo->set_durability(90);
        gearInfo->set_additional_physical_attack(5);
        gearInfo->set_additional_magical_attack(7);
        return slot;
    }
}

class ItemSaveRowsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        data.playerInfo.set_character_id(CHARACTER_ID);

        Protocol::Inventory* inven = data.possession.mutable_inventory();
        for (int32 i = 0; i < SLOT_COUNT; i++)
        {
            *inven->add_gear() = MakeGearSlot(i, 1000 + i, 500 + i);
            *inven->add_consumables() = MakeStackableSlot(i, 2000 + i, 10 + i);
            *inven->add_miscellaneous() = MakeStackableSlot(i, 3000 + i, 20 + i);
        }

        data.gearDirtyFlags = vector<bool>(SLOT_COUNT, false);
        data.consumableDirtyFlags = vector<bool>(SLOT_COUNT, false);
        data.miscDirtyFlags = vector<bool>(SLOT_COUNT, false);
    }

    PlayerSaveData data;
};

TEST_F(ItemSaveRowsTest, OnlyDirtyConsumableSlotsBecomeRows)
{
    (*data.consumableDirtyFlags)[1] = true;

    const auto rows = ItemSaveRows::BuildStackableRows(data, Protocol::ITEM_TYPE_CONSUMABLE);

    ASSERT_TRUE(rows.has_value());
    ASSERT_EQ(rows->size(), 1u) << "바뀌지 않은 슬롯까지 저장하면 안 된다";
    EXPECT_EQ((*rows)[0].characterId, CHARACTER_ID);
    EXPECT_EQ((*rows)[0].slotId, 1);
    EXPECT_EQ((*rows)[0].templateId, 2001);
    EXPECT_EQ((*rows)[0].count, 11);
}

TEST_F(ItemSaveRowsTest, EmptiedSlotBecomesRowWithZeroTemplate)
{
    *data.possession.mutable_inventory()->mutable_consumables(2) = MakeStackableSlot(2, 0, 0);
    (*data.consumableDirtyFlags)[2] = true;

    const auto rows = ItemSaveRows::BuildStackableRows(data, Protocol::ITEM_TYPE_CONSUMABLE);

    ASSERT_TRUE(rows.has_value());
    ASSERT_EQ(rows->size(), 1u) << "비워진 슬롯이 행으로 나가지 않으면 다 쓴 물약이 재접속 뒤 되살아난다";
    EXPECT_EQ((*rows)[0].slotId, 2);
    EXPECT_EQ((*rows)[0].templateId, 0);
}

TEST_F(ItemSaveRowsTest, MiscellaneousUsesItsOwnSlotsAndFlags)
{
    (*data.consumableDirtyFlags)[0] = true;
    (*data.miscDirtyFlags)[2] = true;

    const auto rows = ItemSaveRows::BuildStackableRows(data, Protocol::ITEM_TYPE_MISCELLANEOUS);

    ASSERT_TRUE(rows.has_value());
    ASSERT_EQ(rows->size(), 1u);
    EXPECT_EQ((*rows)[0].slotId, 2);
    EXPECT_EQ((*rows)[0].templateId, 3002);
    EXPECT_EQ((*rows)[0].count, 22);
}

TEST_F(ItemSaveRowsTest, MissingStackableFlagsFails)
{
    data.consumableDirtyFlags.reset();

    EXPECT_FALSE(ItemSaveRows::BuildStackableRows(data, Protocol::ITEM_TYPE_CONSUMABLE).has_value())
        << "더티 플래그가 없는데 빈 결과를 내면 아무것도 저장하지 않고 성공으로 보고한다";
}

TEST_F(ItemSaveRowsTest, GearIsNotStackable)
{
    EXPECT_FALSE(ItemSaveRows::BuildStackableRows(data, Protocol::ITEM_TYPE_GEAR).has_value());
}

TEST_F(ItemSaveRowsTest, DirtyInventoryGearBecomesUnequippedRow)
{
    (*data.gearDirtyFlags)[0] = true;

    const auto rows = ItemSaveRows::BuildGearRows(data);

    ASSERT_TRUE(rows.has_value());
    ASSERT_EQ(rows->size(), 1u);
    const GearSaveRow& row = (*rows)[0];
    EXPECT_EQ(row.characterId, CHARACTER_ID);
    EXPECT_EQ(row.slotId, 0);
    EXPECT_EQ(row.itemUid, 500);
    EXPECT_EQ(row.templateId, 1000);
    EXPECT_FALSE(row.isEquipped);
    EXPECT_EQ(row.enhance, 3);
    EXPECT_EQ(row.durability, 90);
    EXPECT_EQ(row.additionalPhysicalAttack, 5);
    EXPECT_EQ(row.additionalMagicalAttack, 7);
}

TEST_F(ItemSaveRowsTest, DirtyEquippedGearBecomesEquippedRow)
{
    constexpr int32 EQUIP_SLOT_ID = 4;
    (*data.possession.mutable_equipped_gear())[EQUIP_SLOT_ID] = MakeGearSlot(EQUIP_SLOT_ID, 1100, 900);
    data.equippedGearDirtyFlags[EQUIP_SLOT_ID] = true;

    const auto rows = ItemSaveRows::BuildGearRows(data);

    ASSERT_TRUE(rows.has_value());
    ASSERT_EQ(rows->size(), 1u);
    EXPECT_EQ((*rows)[0].slotId, EQUIP_SLOT_ID);
    EXPECT_EQ((*rows)[0].templateId, 1100);
    EXPECT_TRUE((*rows)[0].isEquipped) << "착용 장비와 인벤토리 장비는 같은 slot_id를 쓸 수 있어 is_equipped로 가른다";
}

TEST_F(ItemSaveRowsTest, MissingGearFlagsFails)
{
    data.gearDirtyFlags.reset();

    EXPECT_FALSE(ItemSaveRows::BuildGearRows(data).has_value());
}
