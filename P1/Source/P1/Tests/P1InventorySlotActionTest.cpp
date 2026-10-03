// 인벤토리 칸을 더블클릭했을 때 어떤 요청을 보낼지 정하는 판정을 고정한다.
//
// 왜 이것인가: 무기를 더블클릭해도 장착 요청이 나가지 않는다는 결함이 이 판정에서 생겼다고 보고됐다(#132).
// 블루프린트에 있던 판정을 C++로 옮기면서 아이템 분류 문자열이 아니라 아이템 종류(슬롯 종류)로 가르게 바꿨다.
//
// 기대값은 판정을 다시 계산하지 않고 규칙에서 적었다. 장비 칸은 착용, 소모품 칸은 사용이다. 요구 레벨이
// 플레이어 레벨보다 높으면 요청하지 않는다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Inventory.SlotAction

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Game/Inventory/P1InventorySlotAction.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1InventorySlotActionTest,
    "P1.Inventory.SlotAction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1InventorySlotActionTest::RunTest(const FString& Parameters)
{
    using EKind = FP1InventorySlotAction::EKind;

    // 1) 빈 칸은 종류와 무관하게 요청하지 않는다.
    {
        TestTrue(TEXT("빈 장비 칸"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_INVENTORY_GEAR, 0, 0, 1, false) == EKind::Empty);
        TestTrue(TEXT("빈 소모품 칸"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, 0, 0, 1, false) == EKind::Empty);
    }

    // 2) 소모품 칸의 물약은 사용한다.
    {
        TestTrue(TEXT("물약"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, 2000, 0, 1, false) == EKind::Use);
    }

    // 3) 장비 칸의 아이템은 무기든 방어구든 착용한다. 무기(1005)는 블루프린트의 판정에서 빠졌다고 보고된 경우다.
    {
        TestTrue(TEXT("무기"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_INVENTORY_GEAR, 1005, 1, 1, false) == EKind::Equip);
        TestTrue(TEXT("방어구"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_INVENTORY_GEAR, 1000, 1, 1, false) == EKind::Equip);
    }

    // 4) 요구 레벨이 플레이어 레벨보다 높으면 요청하지 않는다. 같으면 요청한다.
    {
        TestTrue(TEXT("Lv10 무기를 Lv9가 착용"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_INVENTORY_GEAR, 1011, 10, 9, false) == EKind::LevelTooLow);
        TestTrue(TEXT("Lv10 무기를 Lv10이 착용"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_INVENTORY_GEAR, 1011, 10, 10, false) == EKind::Equip);
        TestTrue(TEXT("요구 레벨이 높은 소모품"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, 2000, 5, 4, false) == EKind::LevelTooLow);
    }

    // 5) 기타 칸과 인벤토리가 아닌 칸은 요청하지 않는다. 레벨이 모자라도 레벨 미달이 아니라 이쪽이다.
    {
        TestTrue(TEXT("기타 칸"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_INVENTORY_MISC, 4000, 0, 1, false) == EKind::Unusable);
        TestTrue(TEXT("착용 장비 칸"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_EQUIPPED, 1005, 1, 1, false) == EKind::Unusable);
        TestTrue(TEXT("레벨이 모자란 기타 칸"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_INVENTORY_MISC, 4000, 10, 1, false) == EKind::Unusable);
    }

    // 6) 재사용 대기 중인 소모품은 요청하지 않는다(#133). 장비는 대기가 없으므로 이 인자를 보지 않는다.
    {
        TestTrue(TEXT("대기 중인 물약"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, 2000, 0, 1, true) == EKind::CoolingDown);
        TestTrue(TEXT("대기가 끝난 물약"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, 2000, 0, 1, false) == EKind::Use);
        TestTrue(TEXT("빈 칸은 대기보다 먼저 가린다"),
            FP1InventorySlotAction::Decide(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, 0, 0, 1, true) == EKind::Empty);
    }

    return true;
}

#endif
