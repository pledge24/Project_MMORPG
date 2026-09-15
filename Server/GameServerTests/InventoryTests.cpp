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

/*--------------------------------------------------------------
    매핑 표의 키 집합 고정 (issue #27)

    위의 왕복 테스트는 표의 **내용**이 어긋나면 잡아낸다. 잡아내지 못하는 것은
    표의 **키 집합**이다. .proto에 슬롯 타입이 새로 생기고 아무도 표를 갱신하지
    않아도 지금은 어떤 테스트도 빨개지지 않는다. 실제로 SLOT_TYPE_EQUIPPED와
    SLOT_TYPE_QUICK이 그 상태로 남아 있다.

    그래서 여기서는 열거형의 값 집합을 리플렉션으로 읽어, 아래에 손으로 적어 둔
    기대 집합과 정확히 맞는지 대조한다. 기대 집합은 "표에 있어야 하는 것"과
    "의도적으로 뺀 것"으로 나뉘고, 어느 쪽에도 선언되지 않은 값이 프로토콜에
    나타나면 실패한다. 즉 이 테스트가 빨개지는 것은 "표를 채우라"는 뜻이 아니라,
    **넣을지 말지를 사람이 결정하고 아래 선언을 갱신하라**는 신호다.

    **이 테스트는 표를 채우지 않는다.** 지금의 불완전함을 그대로 고정하는 것이
    목적이므로, 장착 슬롯과 퀵 슬롯은 아래에서 의도적 제외로 선언된다.
    그 둘을 어디에 매핑할지는 `docs/backlog.md`가 후보로 들고 있다.

    ---- 관측 수단과 그 한계 ----

    키가 표에 있는지는 공개 인터페이스로만 관측한다. 내부 상태를 들여다보지 않는다.
    다만 두 표를 각각 따로 들여다볼 수는 없으므로, 관측이 무엇을 보장하는지 적어 둔다.

    GetSlot은 슬롯 타입 표와 조회 표를 잇달아 find로 거친다. 그래서 유효한 slot_id에
    대한 반환값은 **두 표를 합친 결과**다. 널이 아니면 두 표에 키가 다 있다는 뜻이고,
    널이면 둘 중 적어도 하나에 없다는 뜻이다. 조회 표 쪽은 addItem으로도 관측된다 —
    addItem은 아이템 타입을 정한 뒤 조회 표를 find로 확인하고 없으면 거짓을 돌려준다.

    여기서 못 잡는 변경이 둘 남는다. 공개 인터페이스에 관측 경로가 없어 이 티켓에서는
    메우지 않는다. 새 seam을 만드는 일은 #24가 범위 밖으로 둔 리팩토링이다.
      - 슬롯 타입 표에 {SLOT_TYPE_QUICK, ITEM_TYPE_NONE} 같은 항목이 들어와도
        조회 표에 ITEM_TYPE_NONE이 없어 GetSlot은 여전히 널이다.
      - 조회 표에 ITEM_TYPE_NONE이 들어와도 거기에 닿는 슬롯 타입이 없고,
        findFirstAvailableSlotId는 표를 보기 전에 그 값을 거부한다.
    둘 다 "제외하기로 한 값을 표에 넣는" 변경이고, 그 변경을 하는 사람은 아래 선언도
    함께 고치게 된다. 반면 프로토콜에 값이 새로 생기는 경우는 리플렉션 대조가 빠짐없이
    잡는다. 이 티켓이 막으려는 것이 후자다.
---------------------------------------------------------------*/

namespace
{
    // 슬롯 타입 ↔ 아이템 타입 ↔ 그 타입으로 들어가는 시드 아이템.
    // 세 표가 이 짝대로 맞물려 있어야 넣은 아이템이 같은 저장소에서 다시 나온다.
    //
    // 위 INSTANTIATE_TEST_SUITE_P의 목록과 값이 겹치지만 합치지 않는다. 합치려면
    // 기존 SlotCase와 파라미터 목록을 고쳐야 하는데, #27이 기존 줄 수정을 사람 승인
    // 사항으로 두었다. 합치는 일은 후속 작업으로 남긴다.
    struct StoredSlotCase
    {
        Protocol::SlotType slotType;
        Protocol::ItemType itemType;
        int32 templateId;
    };

    // 인벤토리가 저장소를 갖는 슬롯 타입이다.
    // slotTypeToItemTypeMappings와 inventorylookupMappings의 키 집합이 이 선언과 같아야 한다.
    const StoredSlotCase STORED_SLOT_CASES[] = {
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR,
         Protocol::ItemType::ITEM_TYPE_GEAR,
         GEAR_TEMPLATE_ID},
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE,
         Protocol::ItemType::ITEM_TYPE_CONSUMABLE,
         CONSUMABLE_TEMPLATE_ID},
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC,
         Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS,
         MISC_TEMPLATE_ID}};

    // 매핑 표에서 의도적으로 뺀 슬롯 타입의 기대 집합으로는 위 UNKNOWN_SLOT_TYPES(#25)를
    // 그대로 쓴다. 목록을 두 벌로 두면 한쪽만 고쳤을 때 두 테스트가 서로 다른 사실을
    // 주장하게 되므로, 같은 선언을 공유한다.
    //
    // 뺀 이유는 값마다 다음과 같다.
    //  - SLOT_TYPE_NONE     : 슬롯 타입이 정해지지 않았다는 표식이므로 대응할 저장소가 없다.
    //  - SLOT_TYPE_EQUIPPED : 장착 슬롯은 EquippedGear가 따로 관리한다. 인벤토리 표에 넣는 것이
    //                         옳은지부터가 설계 판단이라 이 티켓에서 정하지 않는다 (#24 Out of Scope).
    //  - SLOT_TYPE_QUICK    : 퀵 슬롯은 서버에 대응하는 저장소가 아직 없다. 어떤 아이템 타입에
    //                         매핑할지 결정하는 일이 먼저다.
    // 셋 다 #25에서 "크래시 없이 거부된다"로 고정됐다. 여기서 고정하는 것은 "표에 없다"는 사실이다.

    // 조회 표에서 의도적으로 뺀 아이템 타입이다.
    //  - ITEM_TYPE_NONE : 아이템 타입이 정해지지 않았다는 표식이므로 저장소를 갖지 않는다.
    //                     findFirstAvailableSlotId도 이 값을 명시적으로 거부한다.
    constexpr Protocol::ItemType EXCLUDED_ITEM_TYPES[] = {
        Protocol::ItemType::ITEM_TYPE_NONE};

    // 열거형 배열을 리플렉션이 쓰는 값 번호 목록으로 바꾼다.
    template <typename TEnum, size_t Count>
    vector<int> EnumNumbers(const TEnum (&enumValues)[Count])
    {
        vector<int> numbers;
        numbers.reserve(Count);
        for (const TEnum& enumValue : enumValues)
            numbers.push_back(static_cast<int>(enumValue));

        return numbers;
    }

    // STORED_SLOT_CASES에서 지정한 필드만 뽑아 같은 목록을 만든다.
    template <typename TEnum>
    vector<int> StoredCaseNumbers(TEnum StoredSlotCase::* member)
    {
        vector<int> numbers;
        for (const StoredSlotCase& storedCase : STORED_SLOT_CASES)
            numbers.push_back(static_cast<int>(storedCase.*member));

        return numbers;
    }

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
        {
            EXPECT_TRUE(declared.insert(number).second)
                << "기대 집합 안에서 값(" << number << ")이 중복 선언됐다";
        }
        for (int number : declaredAbsent)
        {
            EXPECT_TRUE(declared.insert(number).second)
                << "기대 집합 안에서 값(" << number << ")이 중복 선언됐다";
        }

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
                << enumDescriptor->name() << " 기대 집합에 프로토콜에서 사라진 값("
                << number << ")이 남아 있다";
        }
    }
}

// 슬롯 타입 매핑 표의 키 집합이 선언된 기대 집합과 일치해야 한다.
TEST_F(InventoryTest, SlotTypeMappingKeySetMatchesDeclaration)
{
    // 헬퍼 안의 ASSERT는 헬퍼만 빠져나오므로, 실패한 줄이 찍히도록 감싼다.
    ASSERT_NO_FATAL_FAILURE(ExpectDeclarationCoversEnum(
        Protocol::SlotType_descriptor(),
        StoredCaseNumbers(&StoredSlotCase::slotType),
        EnumNumbers(UNKNOWN_SLOT_TYPES)));

    // slot_id가 유효 범위 안이므로, 널이 돌아왔다면 이유는 두 표 중 하나에 키가
    // 없다는 것뿐이다. 위 「관측 수단과 그 한계」를 함께 볼 것.
    for (const StoredSlotCase& storedCase : STORED_SLOT_CASES)
    {
        EXPECT_NE(player->inventory->GetSlot(storedCase.slotType, 0), nullptr)
            << Protocol::SlotType_Name(storedCase.slotType) << " 가 매핑 표에서 빠졌다";
    }

    for (Protocol::SlotType excludedSlotType : UNKNOWN_SLOT_TYPES)
    {
        EXPECT_EQ(player->inventory->GetSlot(excludedSlotType, 0), nullptr)
            << Protocol::SlotType_Name(excludedSlotType)
            << " 가 매핑 표에 들어왔다. 의도한 변경이라면 위 기대 집합에서 자리를 옮겨라";
    }
}

// 아이템 타입 조회 표의 키 집합도 같은 방식으로 고정한다.
TEST_F(InventoryTest, ItemTypeLookupKeySetMatchesDeclaration)
{
    ASSERT_NO_FATAL_FAILURE(ExpectDeclarationCoversEnum(
        Protocol::ItemType_descriptor(),
        StoredCaseNumbers(&StoredSlotCase::itemType),
        EnumNumbers(EXCLUDED_ITEM_TYPES)));

    for (const StoredSlotCase& storedCase : STORED_SLOT_CASES)
    {
        SCOPED_TRACE(Protocol::ItemType_Name(storedCase.itemType));

        // addItem은 아이템 타입을 정한 뒤 조회 표를 find로 확인하고, 없으면 거짓을
        // 돌려준다. 그러므로 추가 성공은 "그 아이템 타입의 키가 조회 표에 있다"는 관측이다.
        Protocol::Slot added;
        ASSERT_TRUE(player->inventory->addItem(&added, storedCase.templateId, 1))
            << "조회 표에 이 아이템 타입의 저장소가 없다";

        // 어느 아이템 타입으로 들어갔는지는 더티 플래그가 어느 표에 찍혔는지로 확인한다.
        // 이것까지 봐야 관측한 키가 선언한 아이템 타입이라고 말할 수 있다.
        const vector<bool>& dirtyFlags = player->inventory->GetDirtyFlags(storedCase.itemType);
        EXPECT_TRUE(dirtyFlags[added.slot_id()]) << "선언한 아이템 타입이 아닌 저장소에 아이템이 들어갔다";
    }
}
