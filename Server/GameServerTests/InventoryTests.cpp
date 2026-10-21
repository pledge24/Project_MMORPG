#include "Core/pch.h"
#include <gtest/gtest.h>
#include "PlayerTestAccess.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Inventory/InventoryComponent.h"
#include "Game/Items/InventorySections.h"

/*--------------------------------------------------------------
    인벤토리 구역 테스트

    넣을 때는 아이템 데이터의 "itemType"으로, 꺼낼 때는 칸 주소의 컨테이너로 구역을 찾는다.
    두 경로가 다른 구역을 가리키면 아이템이 엉뚱한 구역으로 샌다. 아래 테스트는 세 구역 전부에 대해
    넣기→조회→지우기 왕복이 같은 구역을 가리키는지 확인한다.

    픽스처 결합도: 아이템 템플릿과 인벤토리 구역 표를 Gamedata::Install로 주입하고 Player를 세션 없이
    EntityFactory로만 만든다. DB·Redis·Room·세션이 필요 없다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 GEAR_TEMPLATE_ID = 1001;
    constexpr int32 CONSUMABLE_TEMPLATE_ID = 2001;
    constexpr int32 MISC_TEMPLATE_ID = 3001;
    /** 장비, 소모품, 기타 구역이 모두 이 칸 수를 갖는다. */
    constexpr int32 SLOT_COUNT = 32;

    /** 테스트가 주입한 표. 시드할 때마다 통째로 다시 설치한다. */
    GamedataTables seededTables;

    void SeedItem(int32 templateId, Protocol::ItemType itemType, int32 maxStack = 99)
    {
        ItemTemplate item;
        item.templateId = templateId;
        item.itemType = itemType;
        item.maxStack = maxStack;
        seededTables.items[templateId] = item;
        Gamedata::Install(seededTables);
    }

    struct SectionCase
    {
        ContainerType container;
        Protocol::ItemType itemType;
        int32 templateId;
        const char* name;
    };

    // 인벤토리가 구역으로 갖는 컨테이너와 그 구역에 드는 아이템 종류, 그 종류로 시드한 아이템.
    // 구역 대응 표가 이 짝과 같아야 넣은 아이템이 같은 구역에서 다시 나온다.
    const SectionCase SECTION_CASES[] = {
        { ContainerType::INVENTORY_GEAR, Protocol::ITEM_TYPE_GEAR, GEAR_TEMPLATE_ID, "Gear" },
        { ContainerType::INVENTORY_CONSUMABLE, Protocol::ITEM_TYPE_CONSUMABLE, CONSUMABLE_TEMPLATE_ID, "Consumable" },
        { ContainerType::INVENTORY_MISC, Protocol::ITEM_TYPE_MISCELLANEOUS, MISC_TEMPLATE_ID, "Misc" },
    };

    // 신뢰 경계 밖에서 온 칸 주소의 컨테이너 가운데 인벤토리 구역이 아닌 것.
    //  - NONE     : 컨테이너가 아닌 칸 종류(퀵슬롯 등)를 읽으면 이 값이 된다.
    //  - EQUIPPED : 착용 장비는 장비 컴포넌트의 컨테이너다. 인벤토리의 구역이 아니다.
    //  - 5        : 열거형에 없는 값. 클라이언트 값을 그대로 바꾼 것이다.
    const ContainerType NON_SECTION_CONTAINERS[] = {
        ContainerType::NONE,
        ContainerType::EQUIPPED,
        static_cast<ContainerType>(5) };

    constexpr int32 LAST_SLOT_INDEX = SLOT_COUNT - 1;
    constexpr int32 OUT_OF_RANGE_SLOT_INDICES[] = { -1, SLOT_COUNT, SLOT_COUNT + 1 };

    // 구역 대응 표에서 의도적으로 뺀 아이템 종류다.
    //  - ITEM_TYPE_NONE : 아이템 종류가 정해지지 않았다는 표식이므로 구역을 갖지 않는다.
    constexpr Protocol::ItemType EXCLUDED_ITEM_TYPES[] = { Protocol::ItemType::ITEM_TYPE_NONE };

    // 선언한 두 집합이 열거형의 값을 빠짐없이, 겹치지 않게 덮는지 확인한다.
    // .proto에 값이 새로 생기면 "어느 쪽에도 선언되지 않은 값"으로 걸린다.
    void ExpectDeclarationCoversEnum(
        const google::protobuf::EnumDescriptor* enumDescriptor,
        const vector<int>& declaredPresent,
        const vector<int>& declaredAbsent)
    {
        ASSERT_NE(enumDescriptor, nullptr);

        set<int> declared;
        for (int number : declaredPresent)
            EXPECT_TRUE(declared.insert(number).second) << "기대 집합 안에서 값(" << number << ")이 중복 선언됐다";
        for (int number : declaredAbsent)
            EXPECT_TRUE(declared.insert(number).second) << "기대 집합 안에서 값(" << number << ")이 중복 선언됐다";

        set<int> declaredInProtocol;
        for (int i = 0; i < enumDescriptor->value_count(); i++)
        {
            const google::protobuf::EnumValueDescriptor* value = enumDescriptor->value(i);
            declaredInProtocol.insert(value->number());

            EXPECT_EQ(declared.count(value->number()), size_t(1))
                << enumDescriptor->name() << "." << value->name()
                << " 가 프로토콜에 새로 생겼다. 표에 넣을지 말지 결정하고 위 기대 집합을 갱신하라";
        }

        for (int number : declared)
        {
            EXPECT_EQ(declaredInProtocol.count(number), size_t(1))
                << enumDescriptor->name() << " 기대 집합에 프로토콜에서 사라진 값(" << number << ")이 남아 있다";
        }
    }
}

class InventoryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        seededTables.inventorySlotCounts = MakeInventorySections(SLOT_COUNT);
        SeedItem(GEAR_TEMPLATE_ID, Protocol::ITEM_TYPE_GEAR);
        SeedItem(CONSUMABLE_TEMPLATE_ID, Protocol::ITEM_TYPE_CONSUMABLE);
        SeedItem(MISC_TEMPLATE_ID, Protocol::ITEM_TYPE_MISCELLANEOUS);

        player = EntityFactory::Create<Player>(PlayerSpawnParams());
        ASSERT_NE(player, nullptr);
    }

    void TearDown() override
    {
        player.reset();
        seededTables = GamedataTables();
        Gamedata::Install(GamedataTables());
    }

    InventoryComponent& Inventory() { return PlayerTestAccess::Inventory(*player); }

    /** 템플릿 번호로 넣고 바뀐 칸을 모두 꺼낸다. 넣지 못하면 빈 목록이다. */
    vector<SlotChange> AddNew(int32 templateId, int32 count)
    {
        vector<SlotChange> changes;
        if (Inventory().AddNewItem(templateId, count))
            Inventory().TakeChanges(changes);

        return changes;
    }

    /** 템플릿 번호로 하나 넣고, 처음 바뀐 칸의 주소를 돌려준다. */
    optional<SlotAddress> AddOne(int32 templateId)
    {
        return PlayerTestAccess::AddNewItem(*player, templateId);
    }

    PlayerRef player;
};

class InventorySectionTest : public InventoryTest, public ::testing::WithParamInterface<SectionCase>
{
};

// 넣은 아이템이 그 종류의 구역에서 다시 보여야 한다.
TEST_P(InventorySectionTest, AddedItemIsFoundInItsSection)
{
    const SectionCase& sectionCase = GetParam();

    optional<SlotAddress> added = AddOne(sectionCase.templateId);
    ASSERT_TRUE(added.has_value());
    EXPECT_EQ(added->container, sectionCase.container) << "아이템이 다른 구역에 들어갔다";

    const ItemInstance* stored = player->GetInventory().FindItem(added.value());
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->templateId, sectionCase.templateId);
    EXPECT_NE(stored->itemUid, 0) << "빈 칸에 새로 놓인 아이템은 UID를 받는다";
}

// 지우기도 같은 구역을 가리켜야 한다.
TEST_P(InventorySectionTest, RemoveItemEmptiesTheSameSlot)
{
    const SectionCase& sectionCase = GetParam();

    optional<SlotAddress> added = AddOne(sectionCase.templateId);
    ASSERT_TRUE(added.has_value());

    ASSERT_TRUE(Inventory().RemoveItem(added.value(), 1));

    vector<SlotChange> changes;
    Inventory().TakeChanges(changes);
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes[0].address, added.value()) << "삭제가 다른 구역을 가리킨다";
    EXPECT_FALSE(changes[0].item.has_value());
    EXPECT_EQ(player->GetInventory().FindItem(added.value()), nullptr);
}

INSTANTIATE_TEST_SUITE_P(
    AllSections,
    InventorySectionTest,
    ::testing::ValuesIn(SECTION_CASES),
    [](const ::testing::TestParamInfo<SectionCase>& info) { return string(info.param.name); });

// 실제 피해: 기타 칸을 지웠는데 장비가 사라진다.
// 두 구역의 0번 칸이 각각 차 있는 상태를 만들고, 기타 쪽만 지운다.
TEST_F(InventoryTest, RemovingMiscItemDoesNotTouchGearSection)
{
    optional<SlotAddress> gearAdded = AddOne(GEAR_TEMPLATE_ID);
    optional<SlotAddress> miscAdded = AddOne(MISC_TEMPLATE_ID);
    ASSERT_TRUE(gearAdded.has_value() && miscAdded.has_value());
    ASSERT_EQ(gearAdded->slotIndex, miscAdded->slotIndex) << "두 구역 모두 0번이 비어 있었어야 한다";

    Inventory().RemoveItem(miscAdded.value(), 1);

    const ItemInstance* gear = player->GetInventory().FindItem(gearAdded.value());
    ASSERT_NE(gear, nullptr) << "기타 칸 삭제가 장비 구역을 지웠다";
    EXPECT_EQ(gear->templateId, GEAR_TEMPLATE_ID);
}

/*--------------------------------------------------------------
    실패한 제거의 부작용
---------------------------------------------------------------*/

// 빈 칸에 제거를 시도한 것만으로 그 칸을 쓸 수 없게 되면 안 된다.
TEST_F(InventoryTest, FailedRemoveLeavesSlotUsable)
{
    ASSERT_FALSE(Inventory().RemoveItem(SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 0 }, 1));

    optional<SlotAddress> added = AddOne(CONSUMABLE_TEMPLATE_ID);
    ASSERT_TRUE(added.has_value());
    EXPECT_EQ(added->slotIndex, 0) << "실패한 제거가 빈 0번 칸을 점유 상태로 만들었다";
}

/*--------------------------------------------------------------
    묶음 상한

    상한을 넘는 수량은 다음 칸으로 넘어가고, 바뀐 칸은 전부 바뀐 칸 목록에
    실린다(S_BUY_ITEM.updated_slots). 다 넣을 수 없으면 아무 칸도 바뀌지 않는다.
---------------------------------------------------------------*/
TEST_F(InventoryTest, StackDoesNotExceedMaxStack)
{
    constexpr int32 MAX_STACK = 10;
    constexpr int32 BUY_COUNT = 15;
    SeedItem(CONSUMABLE_TEMPLATE_ID, Protocol::ITEM_TYPE_CONSUMABLE, MAX_STACK);

    const vector<SlotChange> changes = AddNew(CONSUMABLE_TEMPLATE_ID, BUY_COUNT);
    EXPECT_EQ(changes.size(), 2u) << "바뀐 칸이 목록에 전부 실리지 않았다";

    const ItemInstance* first = player->GetInventory().FindItem(SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 0 });
    const ItemInstance* second = player->GetInventory().FindItem(SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 1 });
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr) << "초과분이 다음 칸으로 넘어가지 않았다";

    EXPECT_LE(first->count, MAX_STACK) << "한 칸에 묶음 상한을 넘겨 쌓았다";
    EXPECT_EQ(first->count + second->count, BUY_COUNT) << "수량이 유실됐다";
}

// 같은 아이템의 묶음에 남은 자리부터 채운다. 더한 개수는 그 묶음의 UID를 따른다.
TEST_F(InventoryTest, NewItemFillsExistingStackFirst)
{
    constexpr int32 MAX_STACK = 10;
    SeedItem(CONSUMABLE_TEMPLATE_ID, Protocol::ITEM_TYPE_CONSUMABLE, MAX_STACK);

    optional<SlotAddress> stack = PlayerTestAccess::AddNewItem(*player, CONSUMABLE_TEMPLATE_ID, 4);
    ASSERT_TRUE(stack.has_value());
    const int64 stackUid = player->GetInventory().FindItem(stack.value())->itemUid;

    const vector<SlotChange> changes = AddNew(CONSUMABLE_TEMPLATE_ID, 3);

    ASSERT_EQ(changes.size(), 1u) << "남은 자리가 있는 묶음을 두고 새 칸을 썼다";
    EXPECT_EQ(changes[0].address, stack.value());
    ASSERT_TRUE(changes[0].item.has_value());
    EXPECT_EQ(changes[0].item->count, 7);
    EXPECT_EQ(changes[0].item->itemUid, stackUid);
}

// 장비는 합치지 않는다. 묶음 상한이 남아 있어도 같은 장비가 든 칸에 더하지 않는다.
TEST_F(InventoryTest, GearIsNeverStacked)
{
    optional<SlotAddress> first = AddOne(GEAR_TEMPLATE_ID);
    ASSERT_TRUE(first.has_value());

    const vector<SlotChange> changes = AddNew(GEAR_TEMPLATE_ID, 1);

    ASSERT_EQ(changes.size(), 1u);
    EXPECT_NE(changes[0].address, first.value()) << "장비를 같은 장비가 든 칸에 쌓았다";
    EXPECT_EQ(player->GetInventory().FindItem(first.value())->count, 1);
}

TEST_F(InventoryTest, AddBeyondCapacityChangesNothing)
{
    constexpr int32 MAX_STACK = 10;
    SeedItem(CONSUMABLE_TEMPLATE_ID, Protocol::ITEM_TYPE_CONSUMABLE, MAX_STACK);

    ASSERT_FALSE(AddNew(CONSUMABLE_TEMPLATE_ID, SLOT_COUNT * MAX_STACK - 1).empty());

    EXPECT_FALSE(Inventory().AddNewItem(CONSUMABLE_TEMPLATE_ID, 2)) << "빈 자리가 1개뿐인데 2개를 받아들였다";

    vector<SlotChange> changes;
    Inventory().TakeChanges(changes);
    EXPECT_TRUE(changes.empty()) << "거절한 추가가 바뀐 칸을 남겼다";

    const ItemInstance* last = player->GetInventory().FindItem(SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, SLOT_COUNT - 1 });
    ASSERT_NE(last, nullptr);
    EXPECT_EQ(last->count, MAX_STACK - 1) << "거절한 추가가 칸 일부를 바꿨다";
}

/*--------------------------------------------------------------
    신뢰 경계 밖에서 온 칸 주소

    RemoveItem과 FindItem이 받는 주소는 클라이언트가 보낸 값이 그대로 들어온다. 인벤토리 구역이 아닌
    컨테이너나 범위 밖 칸 번호가 와도 프로세스가 죽지 않고 정의된 실패로 거부돼야 한다.
---------------------------------------------------------------*/

TEST_F(InventoryTest, NonSectionContainerIsRejected)
{
    for (ContainerType container : NON_SECTION_CONTAINERS)
    {
        SCOPED_TRACE(static_cast<int32>(container));
        const SlotAddress address{ container, 0 };

        EXPECT_FALSE(Inventory().RemoveItem(address, 1));
        EXPECT_EQ(player->GetInventory().FindItem(address), nullptr);
        EXPECT_EQ(player->GetInventory().FindSection(container), nullptr);
    }
}

TEST_F(InventoryTest, OutOfRangeSlotIndexIsRejected)
{
    for (int32 slotIndex : OUT_OF_RANGE_SLOT_INDICES)
    {
        SCOPED_TRACE(slotIndex);
        const SlotAddress address{ ContainerType::INVENTORY_GEAR, slotIndex };

        EXPECT_FALSE(Inventory().RemoveItem(address, 1));
        EXPECT_EQ(player->GetInventory().FindItem(address), nullptr);
    }

    // 경계 안쪽은 그대로 동작해야 한다. 거부가 유효 범위까지 먹으면 안 된다.
    ItemInstance gear;
    gear.itemUid = 77;
    gear.templateId = GEAR_TEMPLATE_ID;
    const SlotAddress lastSlot{ ContainerType::INVENTORY_GEAR, LAST_SLOT_INDEX };
    ASSERT_TRUE(Inventory().LoadItem(lastSlot, gear));
    EXPECT_TRUE(Inventory().RemoveItem(lastSlot, 1)) << "마지막 유효 칸까지 거부됐다";
}

// 거부가 인벤토리를 건드리지 않아야 한다. 같은 입력을 두 번 넣어 판정이 그대로인지 보고,
// 그 뒤 정상 왕복이 되는지로 확인한다.
TEST_F(InventoryTest, RejectedAddressLeavesInventoryUsable)
{
    const SlotAddress rejected{ ContainerType::EQUIPPED, 0 };
    for (int32 attempt = 0; attempt < 2; attempt++)
    {
        EXPECT_FALSE(Inventory().RemoveItem(rejected, 1)) << attempt << "번째 시도에서 판정이 달라졌다";
        EXPECT_EQ(player->GetInventory().FindItem(rejected), nullptr) << attempt << "번째 조회에서 판정이 달라졌다";
    }

    optional<SlotAddress> added = AddOne(GEAR_TEMPLATE_ID);
    ASSERT_TRUE(added.has_value());
    EXPECT_TRUE(Inventory().RemoveItem(added.value(), 1)) << "거부된 요청이 정상 경로를 망가뜨렸다";
}

/*--------------------------------------------------------------
    구역 대응 표의 키 집합 고정

    위의 왕복 테스트는 표의 **내용**이 어긋나면 잡아낸다. 잡아내지 못하는 것은 표의 **키 집합**이다.
    .proto에 아이템 종류가 새로 생기고 아무도 표를 갱신하지 않아도 어떤 테스트도 빨개지지 않는다.
    그래서 열거형의 값 집합을 리플렉션으로 읽어, 위에 손으로 적어 둔 기대 집합과 정확히 맞는지 대조한다.
    이 테스트가 빨개지는 것은 "표를 채우라"는 뜻이 아니라, 넣을지 말지를 사람이 결정하고 그 선언을
    갱신하라는 신호다.
---------------------------------------------------------------*/

TEST_F(InventoryTest, SectionTableCoversEveryItemType)
{
    vector<int> sectionItemTypes;
    for (const SectionCase& sectionCase : SECTION_CASES)
        sectionItemTypes.push_back(sectionCase.itemType);

    vector<int> excludedItemTypes;
    for (Protocol::ItemType itemType : EXCLUDED_ITEM_TYPES)
        excludedItemTypes.push_back(itemType);

    // 헬퍼 안의 ASSERT는 헬퍼만 빠져나오므로, 실패한 줄이 찍히도록 감싼다.
    ASSERT_NO_FATAL_FAILURE(ExpectDeclarationCoversEnum(Protocol::ItemType_descriptor(), sectionItemTypes, excludedItemTypes));

    ASSERT_EQ(size(InventorySections::ALL), size(SECTION_CASES));
    for (const SectionCase& sectionCase : SECTION_CASES)
    {
        SCOPED_TRACE(sectionCase.name);
        EXPECT_EQ(InventorySections::FindContainer(sectionCase.itemType), optional<ContainerType>(sectionCase.container));
        EXPECT_EQ(InventorySections::FindItemType(sectionCase.container), optional<Protocol::ItemType>(sectionCase.itemType));
        EXPECT_NE(player->GetInventory().FindSection(sectionCase.container), nullptr) << "인벤토리가 이 구역을 만들지 않았다";
    }

    for (Protocol::ItemType excluded : EXCLUDED_ITEM_TYPES)
        EXPECT_FALSE(InventorySections::FindContainer(excluded).has_value()) << Protocol::ItemType_Name(excluded) << " 가 구역 표에 들어왔다";

    for (ContainerType container : NON_SECTION_CONTAINERS)
        EXPECT_FALSE(InventorySections::FindItemType(container).has_value()) << static_cast<int32>(container) << " 가 구역 표에 들어왔다";
}

// 종류가 정해지지 않은 아이템은 넣을 구역이 없다.
TEST_F(InventoryTest, ItemWithoutSectionIsRejected)
{
    constexpr int32 EXCLUDED_TEMPLATE_ID = 9001;
    SeedItem(EXCLUDED_TEMPLATE_ID, Protocol::ITEM_TYPE_NONE);

    EXPECT_FALSE(Inventory().AddNewItem(EXCLUDED_TEMPLATE_ID, 1));
}
