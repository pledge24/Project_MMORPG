// 퀵슬롯이 그릴 내용, 특히 소모품 구역 전체의 개수 합계를 고정한다.
//
// 왜 이것인가: 퀵슬롯은 아이템 하나가 아니라 아이템 정의를 가리킨다. 같은 물약이 여러 칸에 나뉘어 있어도 퀵슬롯 하나가
// 전부를 대표하므로, 칸 하나의 개수를 보이면 플레이어가 가진 수보다 적게 보인다. 다 떨어져도 등록은 남고 어둡게 보인다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.QuickSlots.Display

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Game/Items/P1ItemContainer.h"
#include "UI/Common/P1SlotDisplay.h"

#if WITH_DEV_AUTOMATION_TESTS

// 유니티 빌드에서 다른 테스트 파일과 한 번역 단위로 묶여도 도우미 이름이 겹치지 않게 파일 이름의 네임스페이스에 둔다.
namespace P1QuickSlotDisplayTest
{
    constexpr int32 HP_POTION = 2000;
    constexpr int32 MP_POTION = 2001;

    FP1Slot MakeSlot(int32 SlotIndex, int32 TemplateId = 0, int32 Count = 0)
    {
        FP1Slot Slot_;
        Slot_.Address.Container = EP1ContainerType::InventoryConsumable;
        Slot_.Address.SlotIndex = SlotIndex;
        Slot_.Item.ItemUid = TemplateId > 0 ? 100 + SlotIndex : 0;
        Slot_.Item.TemplateId = TemplateId;
        Slot_.Item.Count = Count;
        return Slot_;
    }

    /** 0번과 3번 칸에 HP 물약 3개와 5개, 1번 칸에 MP 물약 2개. */
    UP1ItemContainer* MakeConsumables()
    {
        UP1ItemContainer* Container = NewObject<UP1ItemContainer>(GetTransientPackage());
        Container->Init(EP1ContainerType::InventoryConsumable);
        Container->ResetSlots({ MakeSlot(0, HP_POTION, 3), MakeSlot(1, MP_POTION, 2), MakeSlot(2), MakeSlot(3, HP_POTION, 5) });
        return Container;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1QuickSlotDisplayTest,
    "P1.QuickSlots.Display",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1QuickSlotDisplayTest::RunTest(const FString& Parameters)
{
    using namespace P1QuickSlotDisplayTest;

    // 1) 여러 칸에 나뉜 같은 소모품은 합계로 보인다. 다른 소모품은 세지 않는다.
    {
        UP1ItemContainer* Consumables = MakeConsumables();
        const FP1SlotDisplay Display = FP1SlotDisplay::FromQuickSlotRef(FP1QuickSlotRef::ForItem(HP_POTION), Consumables);

        TestEqual(TEXT("합계: 템플릿"), Display.TemplateId, HP_POTION);
        TestEqual(TEXT("합계: 0번과 3번 칸의 합"), Display.Count, 8);
        TestEqual(TEXT("합계: 재사용 대기는 그 템플릿"), Display.CooldownTemplateId, HP_POTION);
        TestFalse(TEXT("합계: 어둡지 않다"), Display.bDimmed);
        TestTrue(TEXT("합계: 개수를 늘 적는다"), Display.bAlwaysShowCount);

        const FP1SlotDisplay Mp = FP1SlotDisplay::FromQuickSlotRef(FP1QuickSlotRef::ForItem(MP_POTION), Consumables);
        TestEqual(TEXT("합계: 다른 물약은 따로 센다"), Mp.Count, 2);
    }

    // 2) 칸이 바뀐 뒤에는 새 합계로 보인다. 다 떨어져도 등록은 남고 0을 어둡게 보인다.
    {
        UP1ItemContainer* Consumables = MakeConsumables();
        Consumables->ApplySlotChange(MakeSlot(0));
        TestEqual(TEXT("한 묶음을 다 씀: 남은 묶음의 개수"),
            FP1SlotDisplay::FromQuickSlotRef(FP1QuickSlotRef::ForItem(HP_POTION), Consumables).Count, 5);

        Consumables->ApplySlotChange(MakeSlot(3));
        const FP1SlotDisplay Display = FP1SlotDisplay::FromQuickSlotRef(FP1QuickSlotRef::ForItem(HP_POTION), Consumables);
        TestEqual(TEXT("다 떨어짐: 템플릿은 남는다"), Display.TemplateId, HP_POTION);
        TestFalse(TEXT("다 떨어짐: 빈 퀵슬롯이 아니다"), Display.IsEmpty());
        TestEqual(TEXT("다 떨어짐: 개수"), Display.Count, 0);
        TestTrue(TEXT("다 떨어짐: 어둡다"), Display.bDimmed);
    }

    // 3) 빈 퀵슬롯은 빈 칸으로 그린다. 컨테이너가 없으면 개수를 0으로 본다.
    {
        TestTrue(TEXT("빈 퀵슬롯"), FP1SlotDisplay::FromQuickSlotRef(FP1QuickSlotRef(), MakeConsumables()).IsEmpty());

        const FP1SlotDisplay NoContainer = FP1SlotDisplay::FromQuickSlotRef(FP1QuickSlotRef::ForItem(HP_POTION), nullptr);
        TestEqual(TEXT("컨테이너 없음: 개수"), NoContainer.Count, 0);
        TestTrue(TEXT("컨테이너 없음: 어둡다"), NoContainer.bDimmed);
    }

    return true;
}

#endif
