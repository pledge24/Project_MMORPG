#include "pch.h"
#include <gtest/gtest.h>
#include "Player.h"
#include "Inventory.h"

/*--------------------------------------------------------------
    인벤토리 슬롯 타입 매핑 테스트

    Inventory는 아이템 타입을 고르는 표를 세 개 들고 있다.
      - itemTypeMappings            : 아이템 데이터의 "itemType" 문자열 → ItemType  (addItem이 사용)
      - slotTypeToItemTypeMappings  : SlotType → ItemType                           (removeItem/GetSlot이 사용)
      - inventorylookupMappings     : ItemType → 실제 슬롯 배열
    넣을 때와 꺼낼 때가 서로 다른 표를 보므로, 두 표가 어긋나면 아이템이 엉뚱한
    인벤토리로 샌다. 아래 테스트는 세 슬롯 타입 전부에 대해 넣기→조회→지우기
    왕복이 같은 저장소를 가리키는지 확인한다.

    픽스처 결합도: Gamedata::ItemDataTable을 손으로 시드하고 Player를 Init()만
    한다. DB·Redis·Room·세션이 필요 없다 (Player::Init은 Inventory/EquippedGear
    생성이 전부).
---------------------------------------------------------------*/

namespace
{
    constexpr int32 GEAR_TEMPLATE_ID = 1001;
    constexpr int32 CONSUMABLE_TEMPLATE_ID = 2001;
    constexpr int32 MISC_TEMPLATE_ID = 3001;

    void SeedItem(int32 templateId, const char* itemType, int32 maxStack = 99)
    {
        Json data;
        data[string(JsonProperty::Item::ItemType)] = itemType;
        data[string(JsonProperty::Item::MaxStack)] = maxStack;
        Gamedata::ItemDataTable[templateId] = data;
    }

    struct SlotCase
    {
        Protocol::SlotType slotType;
        int32 templateId;
        const char* name;
    };
}

class InventoryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        SeedItem(GEAR_TEMPLATE_ID, "weapon");
        SeedItem(CONSUMABLE_TEMPLATE_ID, "consumption");
        SeedItem(MISC_TEMPLATE_ID, "miscellaneous");

        player = make_shared<Player>();
        ASSERT_TRUE(player->Init());
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::ItemDataTable.clear();
    }

    PlayerRef player;
};

class InventorySlotTypeTest : public InventoryTest, public ::testing::WithParamInterface<SlotCase>
{
};

// 넣은 아이템이 같은 SlotType으로 다시 보여야 한다.
TEST_P(InventorySlotTypeTest, AddedItemIsVisibleThroughGetSlot)
{
    const SlotCase& slotCase = GetParam();

    Protocol::Slot added;
    ASSERT_TRUE(player->inventory->addItem(&added, slotCase.templateId, 1));

    Protocol::Slot* stored = player->inventory->GetSlot(slotCase.slotType, added.slot_id());
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->type(), slotCase.slotType) << "조회가 다른 인벤토리를 가리킨다";
    ASSERT_TRUE(stored->has_item());
    EXPECT_EQ(stored->item().template_id(), slotCase.templateId);
}

// 지우기도 같은 저장소를 가리켜야 한다.
TEST_P(InventorySlotTypeTest, RemoveItemEmptiesTheSameSlot)
{
    const SlotCase& slotCase = GetParam();

    Protocol::Slot added;
    ASSERT_TRUE(player->inventory->addItem(&added, slotCase.templateId, 1));

    Protocol::Slot request;
    request.set_slot_id(added.slot_id());
    request.set_type(slotCase.slotType);

    Protocol::Slot removed;
    ASSERT_TRUE(player->inventory->removeItem(request, &removed, 1));
    EXPECT_EQ(removed.type(), slotCase.slotType) << "삭제가 다른 인벤토리를 가리킨다";
    EXPECT_EQ(removed.state(), Protocol::UpdateState::UPDATE_STATE_REMOVED);
    EXPECT_FALSE(removed.has_item());
}

INSTANTIATE_TEST_SUITE_P(
    AllSlotTypes,
    InventorySlotTypeTest,
    ::testing::Values(
        SlotCase{Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR, GEAR_TEMPLATE_ID, "Gear"},
        SlotCase{Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE, CONSUMABLE_TEMPLATE_ID, "Consumable"},
        SlotCase{Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC, MISC_TEMPLATE_ID, "Misc"}),
    [](const ::testing::TestParamInfo<SlotCase>& info) { return string(info.param.name); });

// 실제 피해: 기타 슬롯을 지웠는데 장비가 사라진다.
// 두 인벤토리의 0번 슬롯이 각각 차 있는 상태를 만들고, 기타 쪽만 지운다.
TEST_F(InventoryTest, RemovingMiscItemDoesNotTouchGearInventory)
{
    Protocol::Slot gearAdded;
    ASSERT_TRUE(player->inventory->addItem(&gearAdded, GEAR_TEMPLATE_ID, 1));

    Protocol::Slot miscAdded;
    ASSERT_TRUE(player->inventory->addItem(&miscAdded, MISC_TEMPLATE_ID, 1));
    ASSERT_EQ(gearAdded.slot_id(), miscAdded.slot_id()) << "두 인벤토리 모두 0번이 비어 있었어야 한다";

    Protocol::Slot request;
    request.set_slot_id(miscAdded.slot_id());
    request.set_type(Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC);

    Protocol::Slot removed;
    player->inventory->removeItem(request, &removed, 1);

    Protocol::Slot* gearSlot =
        player->inventory->GetSlot(Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR, gearAdded.slot_id());
    ASSERT_NE(gearSlot, nullptr);
    ASSERT_TRUE(gearSlot->has_item()) << "기타 슬롯 삭제가 장비 인벤토리를 지웠다";
    EXPECT_EQ(gearSlot->item().template_id(), GEAR_TEMPLATE_ID);
}

/*--------------------------------------------------------------
    실패한 제거의 부작용
---------------------------------------------------------------*/

// 실패한 제거가 슬롯을 더티로 만들면 불필요한 DB 저장·복제가 따라온다.
TEST_F(InventoryTest, FailedRemoveDoesNotMarkSlotDirty)
{
    player->inventory->ClearDirtyFlags();

    Protocol::Slot request;
    request.set_slot_id(0);
    request.set_type(Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE);

    Protocol::Slot removed;
    ASSERT_FALSE(player->inventory->removeItem(request, &removed, 1)) << "빈 슬롯 제거는 실패해야 한다";

    const vector<bool>& flags = player->inventory->GetDirtyFlags(Protocol::ItemType::ITEM_TYPE_CONSUMABLE);
    EXPECT_FALSE(flags[0]) << "실패한 제거가 슬롯을 더티로 만들었다";
}

// 빈 슬롯에 제거를 시도한 것만으로 그 슬롯이 쓸 수 없게 되면 안 된다.
// (removeItem이 검증보다 먼저 mutable_item()을 부르면 has_item()이 켜져 슬롯이 점유된다)
TEST_F(InventoryTest, FailedRemoveLeavesSlotUsable)
{
    Protocol::Slot request;
    request.set_slot_id(0);
    request.set_type(Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE);

    Protocol::Slot removed;
    ASSERT_FALSE(player->inventory->removeItem(request, &removed, 1));

    Protocol::Slot added;
    ASSERT_TRUE(player->inventory->addItem(&added, CONSUMABLE_TEMPLATE_ID, 1));
    EXPECT_EQ(added.slot_id(), 0) << "실패한 제거가 빈 0번 슬롯을 점유 상태로 만들었다";
}

/*--------------------------------------------------------------
    스택 상한 없는 아이템 누적 (tech-debt 참조)

    DISABLED_ 로 둔다. 버그가 아니어서가 아니라, 올바른 수정이 이 세션의
    범위를 넘기 때문이다. 초과분을 다음 슬롯으로 넘기면 한 번의 구매가
    슬롯 두 개를 바꾸는데, S_BUY_ITEM은 `Slot updated_slot` 하나만 나른다
    (Protocol.proto:168-173). 즉 제대로 고치려면 프로토콜을 `repeated`로 바꾸고
    생성기를 다시 돌린 뒤 클라·서버 핸들러를 함께 고쳐야 한다 — 3곳 수정이다.

    버그의 존재는 여기 실행 가능한 형태로 남기고, 수정은 계획된 작업으로 넘긴다.
    확인: GameServerTests.exe --gtest_also_run_disabled_tests
---------------------------------------------------------------*/
TEST_F(InventoryTest, DISABLED_StackDoesNotExceedMaxStack)
{
    constexpr int32 MAX_STACK = 10;
    constexpr int32 BUY_COUNT = 15;
    SeedItem(CONSUMABLE_TEMPLATE_ID, "consumption", MAX_STACK);

    Protocol::Slot added;
    ASSERT_TRUE(player->inventory->addItem(&added, CONSUMABLE_TEMPLATE_ID, BUY_COUNT));

    Protocol::Slot* first =
        player->inventory->GetSlot(Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE, 0);
    Protocol::Slot* second =
        player->inventory->GetSlot(Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE, 1);

    EXPECT_LE(first->item().count(), MAX_STACK) << "한 슬롯에 스택 상한을 넘겨 쌓았다";
    ASSERT_TRUE(second->has_item()) << "초과분이 다음 슬롯으로 넘어가지 않았다";
    EXPECT_EQ(first->item().count() + second->item().count(), BUY_COUNT) << "수량이 유실됐다";
}

/*--------------------------------------------------------------
    신뢰 경계 밖에서 온 슬롯 입력 (issue #25)

    removeItem과 GetSlot이 받는 SlotType과 slot_id는 클라이언트가 보낸 값이
    그대로 들어온다. 매핑 표에 없는 SlotType이나 범위 밖 slot_id가 와도
    프로세스가 죽지 않고 정의된 실패로 거부돼야 한다.

    SLOT_TYPE_EQUIPPED(4)와 SLOT_TYPE_QUICK(5)은 프로토콜에 정의돼 있으면서
    slotTypeToItemTypeMappings에는 없다. 가설이 아니라 실재하는 입력 경로다.
---------------------------------------------------------------*/

namespace
{
    // 프로토콜에는 있으나 slotTypeToItemTypeMappings에는 없는 슬롯 타입들.
    constexpr Protocol::SlotType UNKNOWN_SLOT_TYPES[] = {
        Protocol::SlotType::SLOT_TYPE_NONE,
        Protocol::SlotType::SLOT_TYPE_EQUIPPED,
        Protocol::SlotType::SLOT_TYPE_QUICK};

    constexpr int32 LAST_SLOT_ID = static_cast<int32>(MAX_SLOTS) - 1;
    constexpr int32 OUT_OF_RANGE_SLOT_IDS[] = {
        -1, static_cast<int32>(MAX_SLOTS), static_cast<int32>(MAX_SLOTS) + 1};
}

TEST_F(InventoryTest, RemoveWithUnknownSlotTypeIsRejected)
{
    Protocol::Slot removed;

    for (Protocol::SlotType unknownType : UNKNOWN_SLOT_TYPES)
    {
        Protocol::Slot request;
        request.set_slot_id(0);
        request.set_type(unknownType);

        EXPECT_FALSE(player->inventory->removeItem(request, &removed, 1))
            << "매핑 표에 없는 SlotType(" << unknownType << ") 제거가 거부되지 않았다";
    }
}

TEST_F(InventoryTest, RemoveWithOutOfRangeSlotIdIsRejected)
{
    Protocol::Slot removed;

    for (int32 outOfRangeSlotId : OUT_OF_RANGE_SLOT_IDS)
    {
        Protocol::Slot request;
        request.set_slot_id(outOfRangeSlotId);
        request.set_type(Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR);

        EXPECT_FALSE(player->inventory->removeItem(request, &removed, 1))
            << "범위 밖 slot_id(" << outOfRangeSlotId << ") 제거가 거부되지 않았다";
    }

    // 경계 안쪽은 그대로 동작해야 한다. 거부가 유효 범위까지 먹으면 안 된다.
    Protocol::Item gearInstance;
    gearInstance.set_template_id(GEAR_TEMPLATE_ID);

    Protocol::Slot added;
    ASSERT_TRUE(player->inventory->addItem(&added, gearInstance, 1, LAST_SLOT_ID));

    Protocol::Slot lastSlotRequest;
    lastSlotRequest.set_slot_id(LAST_SLOT_ID);
    lastSlotRequest.set_type(Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR);
    EXPECT_TRUE(player->inventory->removeItem(lastSlotRequest, &removed, 1))
        << "마지막 유효 슬롯까지 거부됐다";
}

TEST_F(InventoryTest, GetSlotWithUnknownSlotTypeReturnsNull)
{
    for (Protocol::SlotType unknownType : UNKNOWN_SLOT_TYPES)
    {
        EXPECT_EQ(player->inventory->GetSlot(unknownType, 0), nullptr)
            << "매핑 표에 없는 SlotType(" << unknownType << ") 조회가 널을 반환하지 않았다";
    }
}

TEST_F(InventoryTest, GetSlotWithOutOfRangeSlotIdReturnsNull)
{
    for (int32 outOfRangeSlotId : OUT_OF_RANGE_SLOT_IDS)
    {
        EXPECT_EQ(player->inventory->GetSlot(Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR, outOfRangeSlotId), nullptr)
            << "범위 밖 slot_id(" << outOfRangeSlotId << ") 조회가 널을 반환하지 않았다";
    }

    // 경계 안쪽은 그대로 동작해야 한다. 거부가 유효 범위까지 먹으면 안 된다.
    EXPECT_NE(player->inventory->GetSlot(Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR, LAST_SLOT_ID), nullptr)
        << "마지막 유효 슬롯 조회까지 거부됐다";
}

// 거부가 인벤토리를 건드리지 않아야 한다. 같은 입력을 두 번 넣어 판정이 그대로인지 보고,
// 그 뒤 정상 왕복이 되는지로 확인한다.
// (표에 키가 삽입됐는지 자체는 공개 인터페이스로 관측할 수 없다 — 삽입되더라도 판정은 같다.
//  삽입을 막는 것은 ToItemType이 find를 쓴다는 사실이고, 여기서 보는 것은 그 결과인 동작이다.)
TEST_F(InventoryTest, RejectedSlotInputLeavesInventoryUsable)
{
    constexpr Protocol::SlotType UNKNOWN_TYPE = Protocol::SlotType::SLOT_TYPE_QUICK;

    Protocol::Slot request;
    request.set_slot_id(0);
    request.set_type(UNKNOWN_TYPE);

    Protocol::Slot removed;
    for (int32 attempt = 0; attempt < 2; attempt++)
    {
        EXPECT_FALSE(player->inventory->removeItem(request, &removed, 1))
            << attempt << "번째 시도에서 판정이 달라졌다";
        EXPECT_EQ(player->inventory->GetSlot(UNKNOWN_TYPE, 0), nullptr)
            << attempt << "번째 조회에서 판정이 달라졌다";
    }

    // 거부가 정상 경로를 망가뜨리지 않았는지 왕복으로 확인한다.
    Protocol::Slot added;
    ASSERT_TRUE(player->inventory->addItem(&added, GEAR_TEMPLATE_ID, 1));

    Protocol::Slot validRequest;
    validRequest.set_slot_id(added.slot_id());
    validRequest.set_type(Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR);
    EXPECT_TRUE(player->inventory->removeItem(validRequest, &removed, 1))
        << "거부된 요청이 정상 경로를 망가뜨렸다";
}
