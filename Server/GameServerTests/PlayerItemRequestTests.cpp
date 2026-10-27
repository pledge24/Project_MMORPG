#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Items/InventorySections.h"
#include "PlayerTestAccess.h"
#include "TestItemTables.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Inventory/InventoryComponent.h"
#include "Game/Equipment/EquipmentComponent.h"
#include "TestLevelTables.h"

/*--------------------------------------------------------------
    아이템 요청 판정 테스트

    클라이언트는 판매·착용·해제 요청에 칸 주소와 그 칸에서 본 아이템의 uid를 보낸다. 요청이 칸 사본을
    통째로 싣고 서버가 그 사본의 아이템으로 판정하던 때는 싼 아이템을 비싼 번호로 팔거나, 아무 장비나 입거나,
    해제하며 다른 아이템을 받을 수 있었다. 판정은 서버의 칸에 든 아이템으로 하고, 요청의 uid는 클라이언트의
    칸이 어긋났는지 대조하는 데만 쓴다.

    픽스처 결합도: 아이템 템플릿, 1레벨짜리 전사 레벨 표를 Gamedata::Install로 주입하고 Player를 세션 없이
    EntityFactory로만 만든다. 시드한 장비에는 물리 공격력만 있다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
    constexpr int32 GREATSWORD_TEMPLATE_ID = 1035;
    constexpr int32 UNSELLABLE_SWORD_TEMPLATE_ID = 1005;
    constexpr int32 UNKNOWN_TEMPLATE_ID = 9999;
    constexpr int32 GEAR_SLOT_COUNT = InventorySections::SLOT_COUNT;

    constexpr int64 START_GOLD = 1000;
    constexpr int32 BASE_PHYSICAL_ATTACK = 5;

    void AddSword(GamedataTables& tables, int32 templateId, int64 buyPrice, int64 sellPrice, bool sellable, int32 physicalAttack)
    {
        TestItemTables::AddGear(tables, templateId, Protocol::GEAR_PART_WEAPON).PhysicalAttack = physicalAttack;

        DesignData::ItemRow& sword = tables.items[templateId];
        sword.BuyPrice = buyPrice;
        sword.SellPrice = sellPrice;
        sword.Sellable = sellable;
    }
}

class PlayerItemRequestTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        AddSword(tables, SWORD_TEMPLATE_ID, 100, 10, true, 10);
        AddSword(tables, GREATSWORD_TEMPLATE_ID, 80000, 8000, true, 500);
        AddSword(tables, UNSELLABLE_SWORD_TEMPLATE_ID, 100, 10, false, 10);
        TestLevelTables::AddWarriorLevel(tables, 1).PhysicalAttack = BASE_PHYSICAL_ATTACK;
        Gamedata::Install(std::move(tables));

        player = EntityFactory::Create<Player>(PlayerSpawnParams());
        ASSERT_NE(player, nullptr);
        PlayerTestAccess::CharacterClass(*player) = Protocol::CHARACTER_CLASS_WARRIOR;
        PlayerTestAccess::PlayerInfo(*player).set_level(1);
        PlayerTestAccess::Gold(*player) = START_GOLD;
        player->SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, BASE_PHYSICAL_ATTACK);
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::Install(GamedataTables());
    }

    // 인벤토리에 아이템을 넣고, 클라이언트가 요청에 실어 보내는 칸 사본을 돌려준다.
    ItemSlotRequest AddToInventory(int32 templateId)
    {
        optional<SlotAddress> address = PlayerTestAccess::AddNewItem(*player, templateId);
        EXPECT_TRUE(address.has_value());
        return address.has_value() ? MakeInventoryRequest(*player, address.value()) : ItemSlotRequest();
    }

    // 칼을 입히고, 클라이언트가 해제를 요청할 때 보내는 장비 칸 사본을 돌려준다.
    ItemSlotRequest EquipSword()
    {
        auto result = player->ProcessEquipGear(AddToInventory(SWORD_TEMPLATE_ID));
        EXPECT_TRUE(result.IsSuccess());
        return MakeEquippedRequest(*player, Protocol::GEAR_PART_WEAPON);
    }

    /** 빈 칸이면 nullptr. */
    const ItemInstance* InventoryItem(const ItemSlotRequest& request)
    {
        return player->GetInventory().FindItem(request.address);
    }

    /** 빈 칸이면 nullptr. */
    const ItemInstance* WeaponItem()
    {
        return player->GetEquipment().FindItem(Protocol::GEAR_PART_WEAPON);
    }

    PlayerRef player;
};

/* 구매 */

TEST_F(PlayerItemRequestTest, BuyingUnknownTemplateIsRejectedWithoutTouchingTable)
{
    EXPECT_EQ(player->ProcessBuyItem(UNKNOWN_TEMPLATE_ID).GetResult(), Protocol::ITEM_RESULT_INVALID_REQUEST);
    EXPECT_EQ(Gamedata::FindItem(UNKNOWN_TEMPLATE_ID), nullptr)
        << "없는 번호를 표에 끼워 넣으면 여러 룸 스레드가 전역 표를 동시에 바꾼다";
    EXPECT_EQ(player->GetGold(), START_GOLD);
}

TEST_F(PlayerItemRequestTest, BuyingBeyondGoldIsRejectedAsNotEnoughGold)
{
    EXPECT_EQ(player->ProcessBuyItem(GREATSWORD_TEMPLATE_ID).GetResult(), Protocol::ITEM_RESULT_NOT_ENOUGH_GOLD);

    EXPECT_EQ(player->GetGold(), START_GOLD);
    EXPECT_EQ(player->GetInventory().FindItem(SlotAddress{ ContainerType::INVENTORY_GEAR, 0 }), nullptr);
}

TEST_F(PlayerItemRequestTest, BuyingIntoFullSectionIsRejectedAsInventoryFull)
{
    for (int32 i = 0; i < GEAR_SLOT_COUNT; i++)
        AddToInventory(SWORD_TEMPLATE_ID);

    EXPECT_EQ(player->ProcessBuyItem(SWORD_TEMPLATE_ID).GetResult(), Protocol::ITEM_RESULT_INVENTORY_FULL);
    EXPECT_EQ(player->GetGold(), START_GOLD) << "넣지 못한 아이템의 값을 받으면 안 된다";
}

/* 판매 */

TEST_F(PlayerItemRequestTest, SellPriceComesFromOwnedItem)
{
    ItemSlotRequest request = AddToInventory(SWORD_TEMPLATE_ID);

    auto result = player->ProcessSellItem(request);
    ASSERT_TRUE(result.IsSuccess());
    EXPECT_EQ(result.GetResult(), Protocol::ITEM_RESULT_SUCCESS);

    EXPECT_EQ(result->gold, START_GOLD + 10);
    EXPECT_EQ(InventoryItem(request), nullptr);
}

TEST_F(PlayerItemRequestTest, SellingWithAnotherItemsUidIsRejected)
{
    ItemSlotRequest request = AddToInventory(SWORD_TEMPLATE_ID);
    const ItemSlotRequest greatswordRequest = AddToInventory(GREATSWORD_TEMPLATE_ID);
    request.itemUid = greatswordRequest.itemUid;

    EXPECT_EQ(player->ProcessSellItem(request).GetResult(), Protocol::ITEM_RESULT_SLOT_CHANGED)
        << "칸의 아이템과 클라이언트가 본 아이템이 다르면 클라이언트의 칸이 어긋난 것이다";

    EXPECT_EQ(player->GetGold(), START_GOLD);
    EXPECT_NE(InventoryItem(request), nullptr);
    EXPECT_NE(InventoryItem(greatswordRequest), nullptr);
}

TEST_F(PlayerItemRequestTest, SellingWithoutUidIsRejected)
{
    ItemSlotRequest request = AddToInventory(SWORD_TEMPLATE_ID);
    request.itemUid = 0;

    EXPECT_EQ(player->ProcessSellItem(request).GetResult(), Protocol::ITEM_RESULT_SLOT_CHANGED) << "uid가 없으면 클라이언트는 빈 칸을 본 것이다";

    EXPECT_EQ(player->GetGold(), START_GOLD);
    EXPECT_NE(InventoryItem(request), nullptr);
}

TEST_F(PlayerItemRequestTest, SellingUnsellableItemIsRejected)
{
    ItemSlotRequest request = AddToInventory(UNSELLABLE_SWORD_TEMPLATE_ID);

    EXPECT_EQ(player->ProcessSellItem(request).GetResult(), Protocol::ITEM_RESULT_NOT_SELLABLE);

    EXPECT_EQ(player->GetGold(), START_GOLD);
    EXPECT_NE(InventoryItem(request), nullptr);
}

/* 착용 */

TEST_F(PlayerItemRequestTest, EquipPutsOwnedItemOnWithItsUid)
{
    ItemSlotRequest request = AddToInventory(SWORD_TEMPLATE_ID);
    const int64 ownedUid = InventoryItem(request)->itemUid;

    auto result = player->ProcessEquipGear(request);
    ASSERT_TRUE(result.IsSuccess());

    ASSERT_NE(WeaponItem(), nullptr);
    EXPECT_EQ(WeaponItem()->templateId, SWORD_TEMPLATE_ID);
    EXPECT_EQ(WeaponItem()->itemUid, ownedUid);
    EXPECT_EQ(InventoryItem(request), nullptr);
}

TEST_F(PlayerItemRequestTest, EquippingWithForgedUidChangesNothing)
{
    ItemSlotRequest request = AddToInventory(SWORD_TEMPLATE_ID);
    request.itemUid += 1;

    auto result = player->ProcessEquipGear(request);
    EXPECT_EQ(result.GetResult(), Protocol::ITEM_RESULT_SLOT_CHANGED) << "uid가 다르면 클라이언트의 칸이 어긋난 것이다";

    EXPECT_EQ(WeaponItem(), nullptr);
    EXPECT_NE(InventoryItem(request), nullptr);
}

TEST_F(PlayerItemRequestTest, EquippingFromEmptySlotChangesNothing)
{
    ItemSlotRequest request;
    request.address = SlotAddress{ ContainerType::INVENTORY_GEAR, 0 };
    request.itemUid = 777;

    auto result = player->ProcessEquipGear(request);
    EXPECT_EQ(result.GetResult(), Protocol::ITEM_RESULT_SLOT_CHANGED) << "빈 칸을 보내고 대검을 공짜로 입으면 안 된다";

    EXPECT_EQ(WeaponItem(), nullptr);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), BASE_PHYSICAL_ATTACK);
}

TEST_F(PlayerItemRequestTest, EquippingOntoOccupiedPartIsRejected)
{
    EquipSword();
    const ItemSlotRequest request = AddToInventory(GREATSWORD_TEMPLATE_ID);

    EXPECT_EQ(player->ProcessEquipGear(request).GetResult(), Protocol::ITEM_RESULT_GEAR_PART_OCCUPIED);

    ASSERT_NE(WeaponItem(), nullptr);
    EXPECT_EQ(WeaponItem()->templateId, SWORD_TEMPLATE_ID);
    EXPECT_NE(InventoryItem(request), nullptr);
}

/* 해제 */

TEST_F(PlayerItemRequestTest, UnequipReturnsEquippedItem)
{
    ItemSlotRequest request = EquipSword();
    const int64 equippedUid = request.itemUid;

    auto result = player->ProcessUnequipGear(request);
    ASSERT_TRUE(result.IsSuccess());

    EXPECT_EQ(WeaponItem(), nullptr);
    const ItemInstance* returned = player->GetInventory().FindItem(SlotAddress{ ContainerType::INVENTORY_GEAR, 0 });
    ASSERT_NE(returned, nullptr);
    EXPECT_EQ(returned->templateId, SWORD_TEMPLATE_ID);
    EXPECT_EQ(returned->itemUid, equippedUid);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), BASE_PHYSICAL_ATTACK);
}

TEST_F(PlayerItemRequestTest, UnequippingWithForgedUidChangesNothing)
{
    ItemSlotRequest request = EquipSword();
    const int64 equippedAttack = player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK);
    request.itemUid += 1;

    auto result = player->ProcessUnequipGear(request);
    EXPECT_EQ(result.GetResult(), Protocol::ITEM_RESULT_SLOT_CHANGED) << "uid가 다르면 클라이언트의 칸이 어긋난 것이다";

    ASSERT_NE(WeaponItem(), nullptr);
    EXPECT_EQ(WeaponItem()->templateId, SWORD_TEMPLATE_ID);
    EXPECT_EQ(player->GetInventory().FindItem(SlotAddress{ ContainerType::INVENTORY_GEAR, 0 }), nullptr);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), equippedAttack);
}

TEST_F(PlayerItemRequestTest, UnequippingIntoFullInventoryChangesNothing)
{
    ItemSlotRequest request = EquipSword();
    const int64 equippedAttack = player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK);
    for (int32 i = 0; i < GEAR_SLOT_COUNT; i++)
        AddToInventory(SWORD_TEMPLATE_ID);

    auto result = player->ProcessUnequipGear(request);
    EXPECT_EQ(result.GetResult(), Protocol::ITEM_RESULT_INVENTORY_FULL) << "넣을 자리가 없는데 장비 칸만 비우면 칼이 사라진다";

    ASSERT_NE(WeaponItem(), nullptr);
    EXPECT_EQ(WeaponItem()->templateId, SWORD_TEMPLATE_ID);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), equippedAttack);
}

TEST_F(PlayerItemRequestTest, UnequippingFromInventorySlotIsInvalidRequest)
{
    const ItemSlotRequest request = AddToInventory(SWORD_TEMPLATE_ID);

    EXPECT_EQ(player->ProcessUnequipGear(request).GetResult(), Protocol::ITEM_RESULT_INVALID_REQUEST)
        << "정상 클라이언트는 착용 장비가 아닌 칸의 해제를 보내지 않는다";
    EXPECT_NE(InventoryItem(request), nullptr);
}

TEST_F(PlayerItemRequestTest, SellingOrEquippingFromEquippedSlotIsInvalidRequest)
{
    const ItemSlotRequest request = EquipSword();

    EXPECT_EQ(player->ProcessSellItem(request).GetResult(), Protocol::ITEM_RESULT_INVALID_REQUEST)
        << "정상 클라이언트는 착용 장비 칸의 판매를 보내지 않는다";
    EXPECT_EQ(player->ProcessEquipGear(request).GetResult(), Protocol::ITEM_RESULT_INVALID_REQUEST);
    EXPECT_NE(WeaponItem(), nullptr);
    EXPECT_EQ(player->GetGold(), START_GOLD);
}

TEST_F(PlayerItemRequestTest, EquippingItemMissingFromTableIsInternalError)
{
    const ItemSlotRequest request = AddToInventory(SWORD_TEMPLATE_ID);
    // 기획 데이터에서 빠진 아이템이 인벤토리에 남아 있다
    GamedataTables tables;
    Gamedata::Install(std::move(tables));

    EXPECT_EQ(player->ProcessEquipGear(request).GetResult(), Protocol::ITEM_RESULT_INTERNAL_ERROR)
        << "칸에 든 아이템은 클라이언트가 고친 값이 아니다";
    EXPECT_NE(InventoryItem(request), nullptr);
}
