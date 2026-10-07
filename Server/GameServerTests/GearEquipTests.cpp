#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Inventory/Inventory.h"
#include "Game/Equipment/EquippedGear.h"

/*--------------------------------------------------------------
    장비 장착·탈착 결과 테스트

    클라는 S_EQUIP_GEAR와 S_UNEQUIP_GEAR의 slot_id와 template_id로 캐릭터 외형을 바꾼다.
    그래서 두 값은 요청 슬롯이 아니라 처리 결과여야 한다. slot_id는 장비 부위, template_id는
    처리 뒤 그 부위의 아이템이다. 다른 플레이어가 보는 외형은 equipped_gear_summary로 간다.

    픽스처 결합도: Gamedata::s_itemDataTable을 손으로 시드하고 Player를 세션 없이 EntityFactory로만 만든다.
    시드한 무기에는 스탯 속성이 없으므로 스탯 계산을 타지 않는다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
}

class GearEquipTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Json sword;
        sword[string(JsonProperty::Item::ItemType)] = "GEAR";
        sword[string(JsonProperty::Item::ItemSubtype)] = string(JsonProperty::Item::GearSubtype_Sword);
        sword[string(JsonProperty::Item::MaxStack)] = 1;
        sword[string(JsonProperty::Item::PhysicalAttack)] = 10;
        Gamedata::s_itemDataTable[SWORD_TEMPLATE_ID] = sword;

        player = EntityFactory::Create<Player>(PlayerSpawnParams());
        ASSERT_NE(player, nullptr);
        player->SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, 5);
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::s_itemDataTable.clear();
    }

    // 인벤토리에 칼을 넣고, 클라가 장착을 요청할 때 보내는 인벤토리 슬롯을 돌려준다.
    Protocol::Slot AddSwordToInventory()
    {
        RepeatedPtrField<Protocol::Slot> addedSlots;
        EXPECT_TRUE(player->_inventory->AddItem(&addedSlots, SWORD_TEMPLATE_ID, 1));
        EXPECT_FALSE(addedSlots.empty());
        return addedSlots.empty() ? Protocol::Slot() : addedSlots[0];
    }

    PlayerRef player;
};

TEST_F(GearEquipTest, EquipReportsGearTypeNotInventorySlot)
{
    Protocol::Slot requestSlot = AddSwordToInventory();

    Protocol::S_EQUIP_GEAR pkt;
    ASSERT_TRUE(player->ProcessEquipGear(requestSlot, pkt));

    EXPECT_EQ(pkt.slot_id(), Protocol::GEAR_TYPE_WEAPON) << "인벤토리 칸 번호가 실려 가면 클라가 무기 부위를 찾지 못한다";
    EXPECT_EQ(pkt.template_id(), SWORD_TEMPLATE_ID);

    const auto& summary = player->_playerInfo->equipped_gear_summary();
    ASSERT_TRUE(summary.contains(Protocol::GEAR_TYPE_WEAPON));
    EXPECT_EQ(summary.at(Protocol::GEAR_TYPE_WEAPON), SWORD_TEMPLATE_ID);
}

TEST_F(GearEquipTest, UnequipReportsEmptiedGearType)
{
    Protocol::S_EQUIP_GEAR equipPkt;
    ASSERT_TRUE(player->ProcessEquipGear(AddSwordToInventory(), equipPkt));

    // 클라는 탈착할 때 장비 슬롯을 그대로 보낸다.
    Protocol::Slot requestSlot = player->_possession->equipped_gear().at(Protocol::GEAR_TYPE_WEAPON);

    Protocol::S_UNEQUIP_GEAR pkt;
    ASSERT_TRUE(player->ProcessUnequipGear(requestSlot, pkt));

    EXPECT_EQ(pkt.slot_id(), Protocol::GEAR_TYPE_WEAPON);
    EXPECT_EQ(pkt.template_id(), 0) << "뺀 아이템의 템플릿이 실려 가면 클라가 그 아이템을 다시 입힌다";
    EXPECT_FALSE(player->_playerInfo->equipped_gear_summary().contains(Protocol::GEAR_TYPE_WEAPON));
}

// DB에서 장착 장비를 불러올 때는 스텟 목록 없이 부른다. 스텟은 CalculateFinalStat이 따로 계산한다.
TEST_F(GearEquipTest, LoadingEquippedGearPlacesItemWithoutTouchingStats)
{
    Protocol::Item sword;
    sword.set_template_id(SWORD_TEMPLATE_ID);

    ASSERT_TRUE(player->_equippedGear->EquipGear(nullptr, nullptr, sword, Protocol::GEAR_TYPE_WEAPON));

    const Protocol::Slot& weaponSlot = player->_possession->equipped_gear().at(Protocol::GEAR_TYPE_WEAPON);
    EXPECT_EQ(weaponSlot.item().template_id(), SWORD_TEMPLATE_ID);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), 5) << "불러오기에서 장비 스텟을 더하면 CalculateFinalStat 검증과 어긋난다";
}
