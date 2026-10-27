#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Items/InventorySections.h"
#include "PlayerTestAccess.h"
#include "TestItemTables.h"
#include "Network/ItemPackets.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Entities/PlayerProgress.h"
#include "TestLevelTables.h"

/*--------------------------------------------------------------
    서버 아이템 타입과 패킷의 변환 테스트

    서버는 아이템과 칸을 전용 타입으로 들고, 패킷을 만들고 읽는 자리에서만 Protocol 타입으로 바꾼다.
    칸에 필드를 더할 때 변환 한쪽을 빠뜨리면 그 값이 패킷에서 사라지므로, 왕복으로 누락을 잡는다.
    입장 패킷은 클라이언트가 칸 목록으로 창의 칸을 만드므로 빈 칸까지 모두 싣는지 본다.
    퀵슬롯도 같은 변환 파일에서 바꾼다. 요청의 참조 종류는 서버 열거형의 범위를 넘는 값을 걸러야 한다.

    픽스처 결합도: 입장 패킷의 소지품만 플레이어가 필요하다. 아이템 템플릿과 레벨 표를
    Gamedata::Install로 주입하고 Player를 세션 없이 EntityFactory로만 만든다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
    constexpr int32 POTION_TEMPLATE_ID = 2000;

    ItemInstance MakeSword()
    {
        return ItemInstance{ 501, SWORD_TEMPLATE_ID, 1, GearDetail{ 3, 80 } };
    }

    ItemInstance MakePotion()
    {
        return ItemInstance{ 502, POTION_TEMPLATE_ID, 7, nullopt };
    }

    // 패킷의 컨테이너 종류와 서버의 컨테이너 종류. 둘 사이의 변환이 이 짝대로여야 요청이 같은 칸을 가리킨다.
    struct ContainerCase
    {
        Protocol::ContainerType protoContainer;
        ContainerType container;
    };
    const ContainerCase CONTAINER_CASES[] = {
        { Protocol::CONTAINER_TYPE_NONE, ContainerType::NONE },
        { Protocol::CONTAINER_TYPE_INVENTORY_GEAR, ContainerType::INVENTORY_GEAR },
        { Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE, ContainerType::INVENTORY_CONSUMABLE },
        { Protocol::CONTAINER_TYPE_INVENTORY_MISC, ContainerType::INVENTORY_MISC },
        { Protocol::CONTAINER_TYPE_EQUIPPED, ContainerType::EQUIPPED },
    };

    /** 패킷의 아이템이 서버 아이템의 번호, uid, 개수를 싣는지 본다. */
    void ExpectCarries(const Protocol::Item& proto, const ItemInstance& item)
    {
        EXPECT_EQ(proto.template_id(), item.templateId);
        EXPECT_EQ(proto.item_uid(), item.itemUid) << "발급하지 않은 uid는 0이다";
        EXPECT_EQ(proto.count(), item.count);
    }

    /** 입장 패킷에서 그 컨테이너의 칸 목록. 없으면 nullptr. */
    const Protocol::ContainerSlots* FindContainer(const Protocol::Possession& possession, Protocol::ContainerType container)
    {
        for (const Protocol::ContainerSlots& containerSlots : possession.containers())
        {
            if (containerSlots.container() == container)
                return &containerSlots;
        }

        return nullptr;
    }
}

/* 아이템 */

TEST(ItemPacketsTest, ItemCarriesTemplateUidAndCount)
{
    const ItemInstance items[] = {
        MakeSword(),
        MakePotion(),
        ItemInstance{ 0, POTION_TEMPLATE_ID, 1, nullopt },
    };

    for (const ItemInstance& item : items)
    {
        SCOPED_TRACE(item.itemUid);
        ExpectCarries(ItemPackets::ToProto(item), item);
    }
}

// 강화 수치와 내구도는 클라이언트가 읽지 않으므로 서버 타입과 DB에만 남는다.
TEST(ItemPacketsTest, GearDetailStaysOnServer)
{
    const Protocol::Item proto = ItemPackets::ToProto(MakeSword());

    EXPECT_EQ(proto.GetDescriptor()->field_count(), 3) << "아이템은 번호, uid, 개수만 싣는다";
    ExpectCarries(proto, MakeSword());
}

/* 컨테이너와 칸 주소 */

TEST(ItemPacketsTest, ContainerSurvivesRoundTrip)
{
    for (const ContainerCase& containerCase : CONTAINER_CASES)
    {
        SCOPED_TRACE(Protocol::ContainerType_Name(containerCase.protoContainer));
        EXPECT_EQ(ItemPackets::FromProto(containerCase.protoContainer), containerCase.container);
        EXPECT_EQ(ItemPackets::ToProto(containerCase.container), containerCase.protoContainer);
    }
}

// .proto에 컨테이너 종류가 새로 생기면 서버의 컨테이너 종류와 짝을 정하고 위 목록을 갱신해야 한다.
TEST(ItemPacketsTest, EveryContainerTypeIsDeclared)
{
    set<int> declared;
    for (const ContainerCase& containerCase : CONTAINER_CASES)
        EXPECT_TRUE(declared.insert(containerCase.protoContainer).second);

    const google::protobuf::EnumDescriptor* descriptor = Protocol::ContainerType_descriptor();
    for (int i = 0; i < descriptor->value_count(); i++)
    {
        EXPECT_EQ(declared.count(descriptor->value(i)->number()), 1u)
            << descriptor->value(i)->name() << " 가 프로토콜에 새로 생겼다. 서버의 컨테이너 종류와 짝을 정하고 위 목록을 갱신하라";
    }
    EXPECT_EQ(declared.size(), size_t(descriptor->value_count())) << "프로토콜에서 사라진 컨테이너 종류가 목록에 남아 있다";
}

TEST(ItemPacketsTest, UnknownContainerTypeBecomesNoContainer)
{
    EXPECT_EQ(ItemPackets::FromProto(static_cast<Protocol::ContainerType>(99)), ContainerType::NONE) << "열거형에 없는 클라이언트 값";
}

TEST(ItemPacketsTest, SlotAddressSurvivesRoundTrip)
{
    const SlotAddress addresses[] = {
        SlotAddress{ ContainerType::INVENTORY_MISC, 0 },
        SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 31 },
        SlotAddress{ ContainerType::EQUIPPED, Protocol::GEAR_PART_WEAPON },
    };

    for (const SlotAddress& address : addresses)
    {
        SCOPED_TRACE(address.slotIndex);
        EXPECT_EQ(ItemPackets::FromProto(ItemPackets::ToProto(address)), address);
    }
}

/* 응답의 칸 변경 */

TEST(ItemPacketsTest, FilledSlotChangeCarriesItsItem)
{
    const SlotChange change{ SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 2 }, MakePotion() };

    const Protocol::SlotChange proto = ItemPackets::ToProto(change);

    EXPECT_EQ(ItemPackets::FromProto(proto.address()), change.address);
    ASSERT_TRUE(proto.has_item());
    ExpectCarries(proto.item(), MakePotion());
}

TEST(ItemPacketsTest, EmptiedSlotChangeCarriesNoItem)
{
    const SlotChange change{ SlotAddress{ ContainerType::EQUIPPED, Protocol::GEAR_PART_WEAPON }, nullopt };

    const Protocol::SlotChange proto = ItemPackets::ToProto(change);

    EXPECT_EQ(ItemPackets::FromProto(proto.address()), change.address);
    EXPECT_FALSE(proto.has_item()) << "아이템이 없는 칸 변경이 그 칸을 비운다";
}

TEST(ItemPacketsTest, EveryChangeIsAddedInOrder)
{
    const vector<SlotChange> changes = {
        SlotChange{ SlotAddress{ ContainerType::INVENTORY_GEAR, 4 }, nullopt },
        SlotChange{ SlotAddress{ ContainerType::EQUIPPED, Protocol::GEAR_PART_WEAPON }, MakeSword() },
    };

    RepeatedPtrField<Protocol::SlotChange> protos;
    ItemPackets::AddChangedSlots(changes, &protos);

    ASSERT_EQ(protos.size(), 2);
    EXPECT_EQ(ItemPackets::FromProto(protos[0].address()), changes[0].address);
    EXPECT_FALSE(protos[0].has_item());
    EXPECT_EQ(ItemPackets::FromProto(protos[1].address()), changes[1].address);
    ASSERT_TRUE(protos[1].has_item());
    ExpectCarries(protos[1].item(), MakeSword());
}

/* 요청의 칸 */

TEST(ItemPacketsTest, RequestPointsAtSlotAndUid)
{
    const ItemSlotRequest request = ItemPackets::ToRequest(ItemPackets::ToProto(SlotAddress{ ContainerType::INVENTORY_GEAR, 3 }), 501);

    EXPECT_EQ(request.address, (SlotAddress{ ContainerType::INVENTORY_GEAR, 3 }));
    EXPECT_EQ(request.itemUid, 501);
}

TEST(ItemPacketsTest, RequestValuesAreNotValidated)
{
    Protocol::SlotAddress address;
    address.set_container(static_cast<Protocol::ContainerType>(99));
    address.set_slot_index(-1);

    const ItemSlotRequest request = ItemPackets::ToRequest(address, 0);

    EXPECT_EQ(request.address, (SlotAddress{ ContainerType::NONE, -1 })) << "값은 검증하지 않고 그대로 옮긴다";
    EXPECT_EQ(request.itemUid, 0);
}

TEST(ItemPacketsTest, RegisteredQuickSlotCarriesItsRef)
{
    const Protocol::QuickSlot proto = ItemPackets::ToProto(QuickSlot{ 3, QuickSlotRef{ QuickSlotRefType::ITEM, POTION_TEMPLATE_ID } });

    EXPECT_EQ(proto.quick_slot_index(), 3);
    ASSERT_TRUE(proto.has_ref());
    EXPECT_EQ(proto.ref().ref_type(), Protocol::QUICK_SLOT_REF_TYPE_ITEM);
    EXPECT_EQ(proto.ref().ref_id(), POTION_TEMPLATE_ID);
}

TEST(ItemPacketsTest, EmptyQuickSlotCarriesNoRef)
{
    const Protocol::QuickSlot proto = ItemPackets::ToProto(QuickSlot{ 0, nullopt });

    EXPECT_EQ(proto.quick_slot_index(), 0);
    EXPECT_FALSE(proto.has_ref()) << "ref가 없으면 빈 퀵슬롯이다";
}

TEST(ItemPacketsTest, QuickSlotsAreAddedInOrder)
{
    RepeatedPtrField<Protocol::QuickSlot> protoQuickSlots;
    ItemPackets::AddQuickSlots({ QuickSlot{ 2, QuickSlotRef{ QuickSlotRefType::ITEM, POTION_TEMPLATE_ID } }, QuickSlot{ 0, nullopt } }, OUT &protoQuickSlots);

    ASSERT_EQ(protoQuickSlots.size(), 2);
    EXPECT_EQ(protoQuickSlots[0].quick_slot_index(), 2);
    EXPECT_EQ(protoQuickSlots[1].quick_slot_index(), 0);
}

TEST(ItemPacketsTest, QuickSlotRefFromRequest)
{
    Protocol::QuickSlotRef item;
    item.set_ref_type(Protocol::QUICK_SLOT_REF_TYPE_ITEM);
    item.set_ref_id(POTION_TEMPLATE_ID);
    EXPECT_EQ(ItemPackets::FromProto(item), (QuickSlotRef{ QuickSlotRefType::ITEM, POTION_TEMPLATE_ID }));

    Protocol::QuickSlotRef none;
    EXPECT_EQ(ItemPackets::FromProto(none), (QuickSlotRef{ QuickSlotRefType::NONE, 0 }));
}

// 서버의 종류는 uint8이다. 그대로 바꾸면 257이 1(아이템)로 읽힌다.
TEST(ItemPacketsTest, UnknownQuickSlotRefTypeIsNotRead)
{
    for (int32 refType : { 2, 257, -1 })
    {
        Protocol::QuickSlotRef ref;
        ref.set_ref_type(static_cast<Protocol::QuickSlotRefType>(refType));
        ref.set_ref_id(POTION_TEMPLATE_ID);

        SCOPED_TRACE(refType);
        EXPECT_EQ(ItemPackets::FromProto(ref), nullopt);
    }
}

/* 입장 패킷의 소지품 */

class ItemPacketsPossessionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        TestItemTables::AddGear(tables, SWORD_TEMPLATE_ID, Protocol::GEAR_PART_WEAPON);
        TestItemTables::AddConsumable(tables, POTION_TEMPLATE_ID, 10);
        TestLevelTables::AddWarriorLevel(tables, 1).MaxHp = 100;
        Gamedata::Install(std::move(tables));

        PlayerProgress progress;
        progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
        progress.level = 1;
        progress.gold = 1234;
        progress.items.push_back(PlacedItem{ SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 1 }, MakePotion() });
        progress.items.push_back(PlacedItem{ SlotAddress{ ContainerType::EQUIPPED, Protocol::GEAR_PART_WEAPON }, MakeSword() });
        // 장비를 가리키는 행과 범위 밖의 행은 아이템 표나 퀵슬롯 개수가 바뀌어 남은 행이다.
        progress.quickSlots.push_back(QuickSlot{ 0, QuickSlotRef{ QuickSlotRefType::ITEM, SWORD_TEMPLATE_ID } });
        progress.quickSlots.push_back(QuickSlot{ 2, QuickSlotRef{ QuickSlotRefType::ITEM, POTION_TEMPLATE_ID } });
        progress.quickSlots.push_back(QuickSlot{ QUICK_SLOT_COUNT, QuickSlotRef{ QuickSlotRefType::ITEM, 2001 } });

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
    EXPECT_EQ(possession.containers().size(), 4) << "인벤토리의 구역 셋과 착용 장비";

    const Protocol::ContainerSlots* gear = FindContainer(possession, Protocol::CONTAINER_TYPE_INVENTORY_GEAR);
    const Protocol::ContainerSlots* consumables = FindContainer(possession, Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE);
    const Protocol::ContainerSlots* misc = FindContainer(possession, Protocol::CONTAINER_TYPE_INVENTORY_MISC);
    ASSERT_NE(gear, nullptr);
    ASSERT_NE(consumables, nullptr);
    ASSERT_NE(misc, nullptr);
    EXPECT_EQ(gear->slots().size(), InventorySections::SLOT_COUNT) << "빈 칸도 실어야 클라이언트가 창의 칸을 만든다";
    EXPECT_EQ(misc->slots().size(), InventorySections::SLOT_COUNT);
    ASSERT_EQ(consumables->slots().size(), InventorySections::SLOT_COUNT);

    for (int32 slotIndex = 0; slotIndex < InventorySections::SLOT_COUNT; slotIndex++)
        EXPECT_EQ(consumables->slots(slotIndex).slot_index(), slotIndex);

    const Protocol::Slot& potion = consumables->slots(1);
    ASSERT_TRUE(potion.has_item());
    ExpectCarries(potion.item(), MakePotion());
    EXPECT_FALSE(consumables->slots(0).has_item());
}

TEST_F(ItemPacketsPossessionTest, CarriesEveryGearPartWithoutPartZero)
{
    const Protocol::Possession possession = ItemPackets::MakePossession(*player);

    const Protocol::ContainerSlots* equipped = FindContainer(possession, Protocol::CONTAINER_TYPE_EQUIPPED);
    ASSERT_NE(equipped, nullptr);
    ASSERT_EQ(equipped->slots().size(), Protocol::GEAR_PART_WEAPON) << "착용 장비에는 0번 칸이 없다";

    for (int32 i = 0; i < equipped->slots().size(); i++)
        EXPECT_EQ(equipped->slots(i).slot_index(), Protocol::GEAR_PART_HELMET + i);

    const Protocol::Slot& weapon = equipped->slots(Protocol::GEAR_PART_WEAPON - 1);
    ASSERT_TRUE(weapon.has_item());
    ExpectCarries(weapon.item(), MakeSword());
    EXPECT_FALSE(equipped->slots(0).has_item());
}

// 응답에 실을 칸 목록은 불러온 칸을 담지 않는다. 담으면 첫 요청의 응답에 함께 실린다.
TEST_F(ItemPacketsPossessionTest, LoadedSlotsAreNotReportedAsChanges)
{
    PlayerTestAccess::Gold(*player) = 1000;

    auto result = player->ProcessBuyItem(POTION_TEMPLATE_ID);
    ASSERT_TRUE(result.IsSuccess());

    ASSERT_EQ(result->changedSlots.size(), 1u);
    EXPECT_EQ(result->changedSlots[0].address, (SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 1 })) << "산 물약은 같은 물약의 묶음에 쌓인다";
}

// 퀵슬롯은 아이템을 담지 않으므로 넣을 수 없는 등록은 버리고 입장시킨다. 다음 저장이 그 행을 지운다.
TEST_F(ItemPacketsPossessionTest, LoadedQuickSlotsDropInvalidRowsAndKeepEntry)
{
    const vector<QuickSlot> quickSlots = player->GetQuickSlots().GetAll();
    ASSERT_EQ(quickSlots.size(), size_t(QUICK_SLOT_COUNT));
    for (const QuickSlot& quickSlot : quickSlots)
    {
        const optional<QuickSlotRef> expected = quickSlot.slotIndex == 2 ? optional(QuickSlotRef{ QuickSlotRefType::ITEM, POTION_TEMPLATE_ID }) : nullopt;
        EXPECT_EQ(quickSlot.ref, expected) << "퀵슬롯 " << quickSlot.slotIndex;
    }
    EXPECT_EQ(player->MakeSaveData().progress.quickSlots, (vector<QuickSlot>{ QuickSlot{ 2, QuickSlotRef{ QuickSlotRefType::ITEM, POTION_TEMPLATE_ID } } }));
}
