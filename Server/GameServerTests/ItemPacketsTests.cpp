#include "Core/pch.h"
#include <gtest/gtest.h>
#include "PlayerTestAccess.h"
#include "Network/ItemPackets.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Entities/PlayerProgress.h"

/*--------------------------------------------------------------
    서버 아이템 타입과 패킷의 변환 테스트

    서버는 아이템과 칸을 전용 타입으로 들고, 패킷을 만들고 읽는 자리에서만 Protocol 타입으로 바꾼다.
    칸에 필드를 더할 때 변환 한쪽을 빠뜨리면 그 값이 패킷에서 사라지므로, 왕복으로 누락을 잡는다.
    입장 패킷은 클라이언트가 칸 목록으로 창의 칸을 만드므로 빈 칸까지 모두 싣는지 본다.

    픽스처 결합도: 입장 패킷의 소지품만 플레이어가 필요하다. 아이템 템플릿, 레벨 표, 인벤토리 구역 표를
    Gamedata::Install로 주입하고 Player를 세션 없이 EntityFactory로만 만든다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
    constexpr int32 POTION_TEMPLATE_ID = 2000;
    constexpr int32 GEAR_SLOT_COUNT = 4;
    constexpr int32 CONSUMABLE_SLOT_COUNT = 3;
    constexpr int32 MISC_SLOT_COUNT = 2;

    ItemInstance MakeSword()
    {
        return ItemInstance{ 501, SWORD_TEMPLATE_ID, 1, GearDetail{ 3, 80 } };
    }

    ItemInstance MakePotion()
    {
        return ItemInstance{ 502, POTION_TEMPLATE_ID, 7, nullopt };
    }

    // 패킷의 칸 종류와 서버의 컨테이너 종류. 둘 사이의 변환이 이 짝대로여야 요청이 같은 칸을 가리킨다.
    struct ContainerCase
    {
        Protocol::SlotType slotType;
        ContainerType container;
    };
    const ContainerCase CONTAINER_CASES[] = {
        { Protocol::SLOT_TYPE_INVENTORY_GEAR, ContainerType::INVENTORY_GEAR },
        { Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, ContainerType::INVENTORY_CONSUMABLE },
        { Protocol::SLOT_TYPE_INVENTORY_MISC, ContainerType::INVENTORY_MISC },
        { Protocol::SLOT_TYPE_EQUIPPED, ContainerType::EQUIPPED },
    };

    // 컨테이너가 아닌 칸 종류.
    //  - SLOT_TYPE_NONE  : 칸 종류가 정해지지 않았다는 표식이다.
    //  - SLOT_TYPE_QUICK : 퀵슬롯은 아이템을 가리키기만 하므로 컨테이너가 아니다.
    constexpr Protocol::SlotType NON_CONTAINER_SLOT_TYPES[] = { Protocol::SLOT_TYPE_NONE, Protocol::SLOT_TYPE_QUICK };
}

/* 아이템 */

TEST(ItemPacketsTest, ItemSurvivesRoundTrip)
{
    const ItemInstance items[] = {
        MakeSword(),
        MakePotion(),
        ItemInstance{ 0, POTION_TEMPLATE_ID, 1, nullopt },
        ItemInstance{ 503, SWORD_TEMPLATE_ID, 1, GearDetail{} },
    };

    for (const ItemInstance& item : items)
    {
        SCOPED_TRACE(item.itemUid);
        EXPECT_EQ(ItemPackets::FromProto(ItemPackets::ToProto(item)), item);
    }
}

TEST(ItemPacketsTest, ItemWithoutUidCarriesNoUid)
{
    const Protocol::Item proto = ItemPackets::ToProto(ItemInstance{ 0, POTION_TEMPLATE_ID, 1, nullopt });

    EXPECT_FALSE(proto.has_item_uid()) << "발급하지 않은 UID를 0으로 보내면 클라이언트가 0번 아이템으로 본다";
    EXPECT_FALSE(proto.has_gearinfo());
}

TEST(ItemPacketsTest, GearCarriesGearInfoWithZeroAdditionalAttack)
{
    const Protocol::Item proto = ItemPackets::ToProto(MakeSword());

    ASSERT_TRUE(proto.has_gearinfo());
    EXPECT_EQ(proto.gearinfo().enhance_level(), 3);
    EXPECT_EQ(proto.gearinfo().durability(), 80);
    EXPECT_EQ(proto.gearinfo().additional_physical_attack(), 0) << "추가 공격력은 서버가 들지 않으므로 0을 채운다";
    EXPECT_EQ(proto.gearinfo().additional_magical_attack(), 0);
}

/* 칸 종류와 컨테이너 */

TEST(ItemPacketsTest, ContainerSurvivesRoundTrip)
{
    for (const ContainerCase& containerCase : CONTAINER_CASES)
    {
        SCOPED_TRACE(Protocol::SlotType_Name(containerCase.slotType));
        EXPECT_EQ(ItemPackets::ToContainerType(containerCase.slotType), containerCase.container);
        EXPECT_EQ(ItemPackets::ToSlotType(containerCase.container), containerCase.slotType);
    }
}

// .proto에 칸 종류가 새로 생기면 변환에 넣을지 말지를 사람이 정하고 위 목록을 갱신해야 한다.
TEST(ItemPacketsTest, EverySlotTypeIsDeclared)
{
    set<int> declared;
    for (const ContainerCase& containerCase : CONTAINER_CASES)
        EXPECT_TRUE(declared.insert(containerCase.slotType).second);
    for (Protocol::SlotType slotType : NON_CONTAINER_SLOT_TYPES)
        EXPECT_TRUE(declared.insert(slotType).second);

    const google::protobuf::EnumDescriptor* descriptor = Protocol::SlotType_descriptor();
    for (int i = 0; i < descriptor->value_count(); i++)
    {
        EXPECT_EQ(declared.count(descriptor->value(i)->number()), 1u)
            << descriptor->value(i)->name() << " 가 프로토콜에 새로 생겼다. 컨테이너인지 정하고 위 목록을 갱신하라";
    }
    EXPECT_EQ(declared.size(), size_t(descriptor->value_count())) << "프로토콜에서 사라진 칸 종류가 목록에 남아 있다";
}

TEST(ItemPacketsTest, NonContainerSlotTypeBecomesNoContainer)
{
    for (Protocol::SlotType slotType : NON_CONTAINER_SLOT_TYPES)
        EXPECT_EQ(ItemPackets::ToContainerType(slotType), ContainerType::NONE) << Protocol::SlotType_Name(slotType);

    EXPECT_EQ(ItemPackets::ToContainerType(static_cast<Protocol::SlotType>(99)), ContainerType::NONE) << "열거형에 없는 클라이언트 값";
}

/* 응답의 바뀐 칸 */

TEST(ItemPacketsTest, FilledChangedSlotCarriesItsItem)
{
    const SlotChange change{ SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 2 }, MakePotion() };

    const Protocol::Slot slot = ItemPackets::ToChangedSlot(change);

    EXPECT_EQ(slot.type(), Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE);
    EXPECT_EQ(slot.slot_id(), 2);
    EXPECT_NE(slot.state(), Protocol::UPDATE_STATE_REMOVED);
    ASSERT_TRUE(slot.has_item());
    EXPECT_EQ(ItemPackets::FromProto(slot.item()), MakePotion());
}

TEST(ItemPacketsTest, EmptiedChangedSlotIsMarkedRemoved)
{
    const SlotChange change{ SlotAddress{ ContainerType::EQUIPPED, Protocol::GEAR_TYPE_WEAPON }, nullopt };

    const Protocol::Slot slot = ItemPackets::ToChangedSlot(change);

    EXPECT_EQ(slot.type(), Protocol::SLOT_TYPE_EQUIPPED);
    EXPECT_EQ(slot.slot_id(), Protocol::GEAR_TYPE_WEAPON);
    EXPECT_FALSE(slot.has_item());
    EXPECT_EQ(slot.state(), Protocol::UPDATE_STATE_REMOVED) << "클라이언트는 이 표시가 있어야 칸을 비운다";
}

/* 요청의 칸 */

TEST(ItemPacketsTest, RequestPointsAtSlotAndItsItem)
{
    Protocol::Slot slot;
    slot.set_type(Protocol::SLOT_TYPE_INVENTORY_GEAR);
    slot.set_slot_id(3);
    *slot.mutable_item() = ItemPackets::ToProto(MakeSword());

    const ItemSlotRequest request = ItemPackets::ToRequest(slot);

    EXPECT_EQ(request.address, (SlotAddress{ ContainerType::INVENTORY_GEAR, 3 }));
    EXPECT_EQ(request.templateId, SWORD_TEMPLATE_ID);
    EXPECT_EQ(request.itemUid, 501);
}

TEST(ItemPacketsTest, RequestWithoutItemPointsAtNothing)
{
    Protocol::Slot slot;
    slot.set_type(Protocol::SLOT_TYPE_QUICK);
    slot.set_slot_id(-1);

    const ItemSlotRequest request = ItemPackets::ToRequest(slot);

    EXPECT_EQ(request.address, (SlotAddress{ ContainerType::NONE, -1 })) << "값은 검증하지 않고 그대로 옮긴다";
    EXPECT_EQ(request.templateId, 0);
    EXPECT_EQ(request.itemUid, 0);
}

/* 입장 패킷의 소지품 */

class ItemPacketsPossessionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ItemTemplate sword;
        sword.templateId = SWORD_TEMPLATE_ID;
        sword.itemType = Protocol::ITEM_TYPE_GEAR;
        sword.gearType = Protocol::GEAR_TYPE_WEAPON;

        ItemTemplate potion;
        potion.templateId = POTION_TEMPLATE_ID;
        potion.itemType = Protocol::ITEM_TYPE_CONSUMABLE;
        potion.maxStack = 10;

        LevelTemplate level1;
        level1.level = 1;
        level1.maxHp = 100;

        GamedataTables tables;
        tables.items[SWORD_TEMPLATE_ID] = sword;
        tables.items[POTION_TEMPLATE_ID] = potion;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1 });
        tables.inventorySlotCounts = {
            { Protocol::ITEM_TYPE_GEAR, GEAR_SLOT_COUNT },
            { Protocol::ITEM_TYPE_CONSUMABLE, CONSUMABLE_SLOT_COUNT },
            { Protocol::ITEM_TYPE_MISCELLANEOUS, MISC_SLOT_COUNT },
        };
        Gamedata::Install(std::move(tables));

        PlayerProgress progress;
        progress.playerInfo.set_class_(Protocol::CLASS_TYPE_WARRIOR);
        progress.playerInfo.set_level(1);
        auto* stats = progress.statInfo.mutable_info();
        for (Protocol::StatType statType : { Protocol::STAT_TYPE_EXP, Protocol::STAT_TYPE_HP, Protocol::STAT_TYPE_MP })
            (*stats)[statType] = 0;
        progress.gold = 1234;
        progress.items.push_back(PlacedItem{ SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 1 }, MakePotion() });
        progress.items.push_back(PlacedItem{ SlotAddress{ ContainerType::EQUIPPED, Protocol::GEAR_TYPE_WEAPON }, MakeSword() });

        PlayerSpawnParams params;
        params.progress = &progress;
        player = EntityFactory::Create<Player>(params);
        ASSERT_NE(player, nullptr);
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::Install(GamedataTables());
    }

    PlayerRef player;
};

TEST_F(ItemPacketsPossessionTest, CarriesGoldAndEveryInventorySlot)
{
    const Protocol::Possession possession = ItemPackets::MakePossession(*player);

    EXPECT_EQ(possession.gold(), 1234);
    ASSERT_EQ(possession.inventory().gear().size(), GEAR_SLOT_COUNT) << "빈 칸도 실어야 클라이언트가 창의 칸을 만든다";
    ASSERT_EQ(possession.inventory().consumables().size(), CONSUMABLE_SLOT_COUNT);
    ASSERT_EQ(possession.inventory().miscellaneous().size(), MISC_SLOT_COUNT);

    for (int32 slotIndex = 0; slotIndex < CONSUMABLE_SLOT_COUNT; slotIndex++)
    {
        const Protocol::Slot& slot = possession.inventory().consumables(slotIndex);
        EXPECT_EQ(slot.type(), Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE);
        EXPECT_EQ(slot.slot_id(), slotIndex);
        EXPECT_EQ(slot.state(), Protocol::UPDATE_STATE_NONE) << "불러온 칸은 변경이 아니다";
    }

    const Protocol::Slot& potion = possession.inventory().consumables(1);
    ASSERT_TRUE(potion.has_item());
    EXPECT_EQ(ItemPackets::FromProto(potion.item()), MakePotion());
    EXPECT_FALSE(possession.inventory().consumables(0).has_item());
}

TEST_F(ItemPacketsPossessionTest, CarriesEveryGearPartWithoutPartZero)
{
    const Protocol::Possession possession = ItemPackets::MakePossession(*player);

    const auto& equipped = possession.equipped_gear();
    EXPECT_FALSE(equipped.contains(Protocol::GEAR_TYPE_NONE)) << "착용 장비에는 0번 칸이 없다";
    for (int32 gearType = Protocol::GEAR_TYPE_HELMET; gearType <= Protocol::GEAR_TYPE_WEAPON; gearType++)
    {
        SCOPED_TRACE(gearType);
        ASSERT_TRUE(equipped.contains(gearType));
        EXPECT_EQ(equipped.at(gearType).type(), Protocol::SLOT_TYPE_EQUIPPED);
        EXPECT_EQ(equipped.at(gearType).slot_id(), gearType);
    }

    const Protocol::Slot& weapon = equipped.at(Protocol::GEAR_TYPE_WEAPON);
    ASSERT_TRUE(weapon.has_item());
    EXPECT_EQ(ItemPackets::FromProto(weapon.item()), MakeSword());
    EXPECT_FALSE(equipped.at(Protocol::GEAR_TYPE_HELMET).has_item());
}

// 응답에 실을 칸 목록은 불러온 칸을 담지 않는다. 담으면 첫 요청의 응답에 함께 실린다.
TEST_F(ItemPacketsPossessionTest, LoadedSlotsAreNotReportedAsChanges)
{
    PlayerTestAccess::Gold(*player) = 1000;

    optional<BuyItemResult> result = player->ProcessBuyItem(POTION_TEMPLATE_ID);
    ASSERT_TRUE(result.has_value());

    ASSERT_EQ(result->changedSlots.size(), 1u);
    EXPECT_EQ(result->changedSlots[0].address, (SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 1 })) << "산 물약은 같은 물약의 묶음에 쌓인다";
}
