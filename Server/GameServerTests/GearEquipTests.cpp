#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Inventory/InventoryComponent.h"
#include "Game/Equipment/EquipmentComponent.h"

/*--------------------------------------------------------------
    장비 장착·탈착 결과 테스트

    클라는 S_EQUIP_GEAR와 S_UNEQUIP_GEAR의 slot_id와 template_id로 캐릭터 외형을 바꾼다.
    그래서 두 값은 요청 슬롯이 아니라 처리 결과여야 한다. slot_id는 장비 부위, template_id는
    처리 뒤 그 부위의 아이템이다. 다른 플레이어가 보는 외형은 equipped_gear_summary로 간다.

    픽스처 결합도: 아이템 템플릿과 1레벨짜리 전사 레벨 표를 Gamedata::Install로 주입하고 Player를 세션 없이
    EntityFactory로만 만든다. 장착과 해제가 레벨 표로 최종 스탯을 다시 계산하므로 레벨 표가 있어야 한다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
    constexpr int32 BASE_PHYSICAL_ATTACK = 5;
}

class GearEquipTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ItemTemplate sword;
        sword.templateId = SWORD_TEMPLATE_ID;
        sword.itemType = Protocol::ITEM_TYPE_GEAR;
        sword.gearType = Protocol::GEAR_TYPE_WEAPON;
        sword.physicalAttack = 10;

        LevelTemplate level1;
        level1.level = 1;
        level1.physicalAttack = BASE_PHYSICAL_ATTACK;

        GamedataTables tables;
        tables.items[SWORD_TEMPLATE_ID] = sword;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1 });
        Gamedata::Install(std::move(tables));

        player = EntityFactory::Create<Player>(PlayerSpawnParams());
        ASSERT_NE(player, nullptr);
        player->_playerInfo->set_class_(Protocol::CLASS_TYPE_WARRIOR);
        player->_playerInfo->set_level(1);
        player->SetStatValue(Protocol::STAT_TYPE_HP, 0);
        player->SetStatValue(Protocol::STAT_TYPE_MP, 0);
        player->SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, BASE_PHYSICAL_ATTACK);
        player->SetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK, 0);
        ASSERT_TRUE(player->OnLoaded()) << "입장을 마친 플레이어처럼 최대치까지 계산해 둔다";
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::Install(GamedataTables());
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

// 장착과 해제는 레벨 표의 기본 스탯과 착용 장비의 합으로 최종 스탯을 다시 계산하고, 바뀐 스탯만 응답에 싣는다.
TEST_F(GearEquipTest, EquipAndUnequipReportRecalculatedStats)
{
    Protocol::S_EQUIP_GEAR equipPkt;
    ASSERT_TRUE(player->ProcessEquipGear(AddSwordToInventory(), equipPkt));

    ASSERT_EQ(equipPkt.updated_stat_size(), 1) << "값이 바뀌지 않은 스탯까지 실으면 클라이언트가 불필요하게 갱신한다";
    EXPECT_EQ(equipPkt.updated_stat(0).type(), Protocol::STAT_TYPE_PHYSICAL_ATTACK);
    EXPECT_EQ(equipPkt.updated_stat(0).value(), BASE_PHYSICAL_ATTACK + 10);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), BASE_PHYSICAL_ATTACK + 10);

    Protocol::S_UNEQUIP_GEAR unequipPkt;
    ASSERT_TRUE(player->ProcessUnequipGear(player->_possession->equipped_gear().at(Protocol::GEAR_TYPE_WEAPON), unequipPkt));

    ASSERT_EQ(unequipPkt.updated_stat_size(), 1);
    EXPECT_EQ(unequipPkt.updated_stat(0).value(), BASE_PHYSICAL_ATTACK);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), BASE_PHYSICAL_ATTACK);
}

// DB에서 장착 장비를 불러올 때는 스텟을 바꾸지 않는다. 스텟은 OnLoaded가 대조한 뒤 RefreshFinalStat이 계산한다.
TEST_F(GearEquipTest, LoadingEquippedGearPlacesItemWithoutTouchingStats)
{
    Protocol::Item sword;
    sword.set_template_id(SWORD_TEMPLATE_ID);

    ASSERT_TRUE(player->_equipment->LoadEquipped(sword, Protocol::GEAR_TYPE_WEAPON));

    const Protocol::Slot& weaponSlot = player->_possession->equipped_gear().at(Protocol::GEAR_TYPE_WEAPON);
    EXPECT_EQ(weaponSlot.item().template_id(), SWORD_TEMPLATE_ID);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), BASE_PHYSICAL_ATTACK) << "불러오기에서 장비 스텟을 더하면 OnLoaded의 대조와 어긋난다";
}
