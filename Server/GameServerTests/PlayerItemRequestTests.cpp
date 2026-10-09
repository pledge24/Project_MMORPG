#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Inventory/InventoryComponent.h"
#include "Game/Equipment/EquipmentComponent.h"

/*--------------------------------------------------------------
    아이템 요청 판정 테스트

    클라이언트는 판매·착용·해제 요청에 자기 슬롯 사본을 통째로 보낸다. 서버가 그 사본의 아이템으로
    판정하던 때는 싼 아이템을 비싼 번호로 팔거나, 아무 장비나 입거나, 해제하며 다른 아이템을 받을 수
    있었다. 판정은 서버 슬롯에 든 아이템으로 하고, 요청의 아이템은 클라이언트 슬롯이 어긋났는지
    대조하는 데만 쓴다.

    픽스처 결합도: 아이템 템플릿을 Gamedata::Install로 주입하고 Player를 세션 없이 EntityFactory로만 만든다.
    시드한 장비에는 물리 공격력만 있다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
    constexpr int32 GREATSWORD_TEMPLATE_ID = 1035;
    constexpr int32 UNSELLABLE_SWORD_TEMPLATE_ID = 1005;
    constexpr int32 UNKNOWN_TEMPLATE_ID = 9999;

    constexpr int64 START_GOLD = 1000;
    constexpr int64 BASE_PHYSICAL_ATTACK = 5;

    ItemTemplate MakeSword(int32 templateId, int64 buyPrice, int64 sellPrice, bool sellable, int32 physicalAttack)
    {
        ItemTemplate sword;
        sword.templateId = templateId;
        sword.itemType = Protocol::ITEM_TYPE_GEAR;
        sword.gearType = Protocol::GEAR_TYPE_WEAPON;
        sword.buyPrice = buyPrice;
        sword.sellPrice = sellPrice;
        sword.sellable = sellable;
        sword.physicalAttack = physicalAttack;
        return sword;
    }
}

class PlayerItemRequestTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        tables.items[SWORD_TEMPLATE_ID] = MakeSword(SWORD_TEMPLATE_ID, 100, 10, true, 10);
        tables.items[GREATSWORD_TEMPLATE_ID] = MakeSword(GREATSWORD_TEMPLATE_ID, 80000, 8000, true, 500);
        tables.items[UNSELLABLE_SWORD_TEMPLATE_ID] = MakeSword(UNSELLABLE_SWORD_TEMPLATE_ID, 100, 10, false, 10);
        Gamedata::Install(std::move(tables));

        player = EntityFactory::Create<Player>(PlayerSpawnParams());
        ASSERT_NE(player, nullptr);
        player->_possession->set_gold(START_GOLD);
        player->SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, BASE_PHYSICAL_ATTACK);
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::Install(GamedataTables());
    }

    // 인벤토리에 아이템을 넣고, 클라이언트가 요청에 실어 보내는 슬롯 사본을 돌려준다.
    Protocol::Slot AddToInventory(int32 templateId)
    {
        RepeatedPtrField<Protocol::Slot> addedSlots;
        EXPECT_TRUE(player->_inventory->AddItem(&addedSlots, templateId, 1));
        return addedSlots.empty() ? Protocol::Slot() : addedSlots[0];
    }

    // 칼을 입히고, 클라이언트가 해제를 요청할 때 보내는 장비 슬롯 사본을 돌려준다.
    Protocol::Slot EquipSword()
    {
        Protocol::S_EQUIP_GEAR pkt;
        EXPECT_TRUE(player->ProcessEquipGear(AddToInventory(SWORD_TEMPLATE_ID), pkt));
        return *player->_equipment->GetSlot(Protocol::GEAR_TYPE_WEAPON);
    }

    const Protocol::Slot& InventorySlot(const Protocol::Slot& slot)
    {
        return *player->_inventory->GetSlot(slot.type(), slot.slot_id());
    }

    const Protocol::Slot& WeaponSlot()
    {
        return *player->_equipment->GetSlot(Protocol::GEAR_TYPE_WEAPON);
    }

    PlayerRef player;
};

/* 구매 */

TEST_F(PlayerItemRequestTest, BuyingUnknownTemplateIsRejectedWithoutTouchingTable)
{
    RepeatedPtrField<Protocol::Slot> updatedSlots;
    int64 totalGold = 0;

    EXPECT_FALSE(player->ProcessBuyItem(&updatedSlots, totalGold, UNKNOWN_TEMPLATE_ID));
    EXPECT_EQ(Gamedata::FindItem(UNKNOWN_TEMPLATE_ID), nullptr)
        << "없는 번호를 표에 끼워 넣으면 여러 룸 스레드가 전역 표를 동시에 바꾼다";
    EXPECT_EQ(player->_possession->gold(), START_GOLD);
}

/* 판매 */

TEST_F(PlayerItemRequestTest, SellPriceComesFromOwnedItem)
{
    Protocol::Slot requestSlot = AddToInventory(SWORD_TEMPLATE_ID);

    Protocol::Slot updatedSlot;
    int64 totalGold = 0;
    ASSERT_TRUE(player->ProcessSellItem(requestSlot, &updatedSlot, totalGold));

    EXPECT_EQ(totalGold, START_GOLD + 10);
    EXPECT_FALSE(InventorySlot(requestSlot).has_item());
}

TEST_F(PlayerItemRequestTest, SellingWithForgedTemplateIsRejected)
{
    Protocol::Slot requestSlot = AddToInventory(SWORD_TEMPLATE_ID);
    requestSlot.mutable_item()->set_template_id(GREATSWORD_TEMPLATE_ID);

    Protocol::Slot updatedSlot;
    int64 totalGold = 0;
    EXPECT_FALSE(player->ProcessSellItem(requestSlot, &updatedSlot, totalGold))
        << "싼 칼을 비싼 칼 번호로 팔면 골드가 생긴다";

    EXPECT_EQ(player->_possession->gold(), START_GOLD);
    EXPECT_TRUE(InventorySlot(requestSlot).has_item());
}

TEST_F(PlayerItemRequestTest, SellingUnsellableItemIsRejected)
{
    Protocol::Slot requestSlot = AddToInventory(UNSELLABLE_SWORD_TEMPLATE_ID);

    Protocol::Slot updatedSlot;
    int64 totalGold = 0;
    EXPECT_FALSE(player->ProcessSellItem(requestSlot, &updatedSlot, totalGold));

    EXPECT_EQ(player->_possession->gold(), START_GOLD);
    EXPECT_TRUE(InventorySlot(requestSlot).has_item());
}

/* 착용 */

TEST_F(PlayerItemRequestTest, EquipPutsOwnedItemOnWithItsUid)
{
    Protocol::Slot requestSlot = AddToInventory(SWORD_TEMPLATE_ID);
    const int64 ownedUid = InventorySlot(requestSlot).item().item_uid();

    Protocol::S_EQUIP_GEAR pkt;
    ASSERT_TRUE(player->ProcessEquipGear(requestSlot, pkt));

    EXPECT_EQ(WeaponSlot().item().template_id(), SWORD_TEMPLATE_ID);
    EXPECT_EQ(WeaponSlot().item().item_uid(), ownedUid);
    EXPECT_FALSE(InventorySlot(requestSlot).has_item());
}

TEST_F(PlayerItemRequestTest, EquippingWithForgedTemplateChangesNothing)
{
    Protocol::Slot requestSlot = AddToInventory(SWORD_TEMPLATE_ID);
    requestSlot.mutable_item()->set_template_id(GREATSWORD_TEMPLATE_ID);

    Protocol::S_EQUIP_GEAR pkt;
    EXPECT_FALSE(player->ProcessEquipGear(requestSlot, pkt)) << "인벤토리의 칼 대신 요청에 실린 대검을 입으면 안 된다";

    EXPECT_FALSE(WeaponSlot().has_item());
    EXPECT_TRUE(InventorySlot(requestSlot).has_item());
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), BASE_PHYSICAL_ATTACK);
}

TEST_F(PlayerItemRequestTest, EquippingWithForgedUidChangesNothing)
{
    Protocol::Slot requestSlot = AddToInventory(SWORD_TEMPLATE_ID);
    requestSlot.mutable_item()->set_item_uid(requestSlot.item().item_uid() + 1);

    Protocol::S_EQUIP_GEAR pkt;
    EXPECT_FALSE(player->ProcessEquipGear(requestSlot, pkt)) << "uid가 다르면 클라이언트 슬롯이 어긋난 것이다";

    EXPECT_FALSE(WeaponSlot().has_item());
    EXPECT_TRUE(InventorySlot(requestSlot).has_item());
}

TEST_F(PlayerItemRequestTest, EquippingFromEmptySlotChangesNothing)
{
    Protocol::Slot requestSlot;
    requestSlot.set_type(Protocol::SLOT_TYPE_INVENTORY_GEAR);
    requestSlot.set_slot_id(0);
    requestSlot.mutable_item()->set_template_id(GREATSWORD_TEMPLATE_ID);

    Protocol::S_EQUIP_GEAR pkt;
    EXPECT_FALSE(player->ProcessEquipGear(requestSlot, pkt)) << "빈 칸을 보내고 대검을 공짜로 입으면 안 된다";

    EXPECT_FALSE(WeaponSlot().has_item());
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), BASE_PHYSICAL_ATTACK);
}

/* 해제 */

TEST_F(PlayerItemRequestTest, UnequipReturnsEquippedItem)
{
    Protocol::Slot requestSlot = EquipSword();
    const int64 equippedUid = requestSlot.item().item_uid();

    Protocol::S_UNEQUIP_GEAR pkt;
    ASSERT_TRUE(player->ProcessUnequipGear(requestSlot, pkt));

    EXPECT_FALSE(WeaponSlot().has_item());
    const Protocol::Slot* returnedSlot = player->_inventory->GetSlot(Protocol::SLOT_TYPE_INVENTORY_GEAR, 0);
    ASSERT_TRUE(returnedSlot->has_item());
    EXPECT_EQ(returnedSlot->item().template_id(), SWORD_TEMPLATE_ID);
    EXPECT_EQ(returnedSlot->item().item_uid(), equippedUid);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), BASE_PHYSICAL_ATTACK);
}

TEST_F(PlayerItemRequestTest, UnequippingWithForgedTemplateChangesNothing)
{
    Protocol::Slot requestSlot = EquipSword();
    const int64 equippedAttack = player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK);
    requestSlot.mutable_item()->set_template_id(GREATSWORD_TEMPLATE_ID);

    Protocol::S_UNEQUIP_GEAR pkt;
    EXPECT_FALSE(player->ProcessUnequipGear(requestSlot, pkt)) << "칼을 빼고 대검을 돌려받으면 안 된다";

    EXPECT_EQ(WeaponSlot().item().template_id(), SWORD_TEMPLATE_ID);
    EXPECT_FALSE(player->_inventory->GetSlot(Protocol::SLOT_TYPE_INVENTORY_GEAR, 0)->has_item());
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), equippedAttack);
}

TEST_F(PlayerItemRequestTest, UnequippingIntoFullInventoryChangesNothing)
{
    Protocol::Slot requestSlot = EquipSword();
    const int64 equippedAttack = player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK);
    for (int32 i = 0; i < MAX_SLOTS; i++)
        AddToInventory(SWORD_TEMPLATE_ID);

    Protocol::S_UNEQUIP_GEAR pkt;
    EXPECT_FALSE(player->ProcessUnequipGear(requestSlot, pkt)) << "넣을 자리가 없는데 장비 칸만 비우면 칼이 사라진다";

    EXPECT_EQ(WeaponSlot().item().template_id(), SWORD_TEMPLATE_ID);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), equippedAttack);
}
