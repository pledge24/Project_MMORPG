// 클라이언트 컨테이너가 칸을 고친 뒤에 알리는 규칙과, 유효하지 않은 칸 번호를 버리는 규칙을 고정한다.
//
// 왜 이것인가: 칸 사본과 위젯이 같은 델리게이트의 형제 구독자였다. 구독 순서에 따라 위젯이 사본보다 먼저 불려
// 고쳐지기 전의 값을 읽을 수 있었다. 퀵슬롯처럼 구역 전체의 합계를 읽는 화면이 이 순서에 기댄다.
// 또 칸 사본은 서버의 칸 번호를 범위 검사 없이 배열 인덱스로 썼다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Items.ItemContainer

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Game/Items/P1ItemContainer.h"

#if WITH_DEV_AUTOMATION_TESTS

// 유니티 빌드에서 다른 테스트 파일과 한 번역 단위로 묶여도 도우미 이름이 겹치지 않게 파일 이름의 네임스페이스에 둔다.
namespace P1ItemContainerTest
{
    FP1Slot MakeSlot(EP1ContainerType Container, int32 SlotIndex, int32 TemplateId = 0, int32 Count = 0, int64 ItemUid = 0)
    {
        FP1Slot Slot_;
        Slot_.Address.Container = Container;
        Slot_.Address.SlotIndex = SlotIndex;
        Slot_.Item.ItemUid = ItemUid;
        Slot_.Item.TemplateId = TemplateId;
        Slot_.Item.Count = Count;
        return Slot_;
    }

    /** 칸 0~2를 가진 소모품 구역. 0번 칸에 물약 3개, 2번 칸에 물약 5개. */
    UP1ItemContainer* MakeConsumableContainer()
    {
        UP1ItemContainer* Container = NewObject<UP1ItemContainer>(GetTransientPackage());
        Container->Init(EP1ContainerType::InventoryConsumable);
        Container->ResetSlots({
            MakeSlot(EP1ContainerType::InventoryConsumable, 0, 2000, 3, 11),
            MakeSlot(EP1ContainerType::InventoryConsumable, 1),
            MakeSlot(EP1ContainerType::InventoryConsumable, 2, 2000, 5, 12),
        });
        return Container;
    }

    int32 SumCounts(const UP1ItemContainer& Container, int32 TemplateId)
    {
        int32 Sum = 0;
        for (const FP1Slot& Slot_ : Container.GetSlots())
        {
            if (Slot_.Item.TemplateId == TemplateId)
                Sum += Slot_.Item.Count;
        }
        return Sum;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1ItemContainerTest,
    "P1.Items.ItemContainer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1ItemContainerTest::RunTest(const FString& Parameters)
{
    using namespace P1ItemContainerTest;

    // 1) 칸 전체를 바꾸면 칸 번호 순서로 들고, 주소의 컨테이너를 자기 종류로 채운다.
    {
        UP1ItemContainer* Container = NewObject<UP1ItemContainer>(GetTransientPackage());
        Container->Init(EP1ContainerType::Equipped);
        Container->ResetSlots({
            MakeSlot(EP1ContainerType::None, 6, 1005, 1, 21),
            MakeSlot(EP1ContainerType::None, 1),
            MakeSlot(EP1ContainerType::None, 3, 1000, 1, 22),
        });

        const TArray<FP1Slot>& Slots = Container->GetSlots();
        if (TestEqual(TEXT("칸 수"), Slots.Num(), 3))
        {
            TestEqual(TEXT("첫 칸 번호"), Slots[0].Address.SlotIndex, 1);
            TestEqual(TEXT("둘째 칸 번호"), Slots[1].Address.SlotIndex, 3);
            TestEqual(TEXT("셋째 칸 번호"), Slots[2].Address.SlotIndex, 6);
            TestTrue(TEXT("주소의 컨테이너"), Slots[2].Address.Container == EP1ContainerType::Equipped);
        }

        const FP1Slot* Weapon = Container->FindSlot(6);
        if (TestNotNull(TEXT("6번 칸"), Weapon))
            TestEqual(TEXT("6번 칸의 아이템"), Weapon->Item.TemplateId, 1005);
        TestNull(TEXT("받지 않은 칸 번호"), Container->FindSlot(2));
    }

    // 2) 칸 하나를 고친 뒤에 알린다. 알림 안에서 읽은 칸과 구역 합계가 새 값이다.
    {
        UP1ItemContainer* Container = MakeConsumableContainer();

        int32 Notified = 0;
        int32 CountInDelegate = -1;
        int32 SumInDelegate = -1;
        FP1Slot NotifiedSlot;
        Container->OnSlotChanged.AddLambda([&](const FP1Slot& Changed)
        {
            ++Notified;
            NotifiedSlot = Changed;
            if (const FP1Slot* Found = Container->FindSlot(Changed.Address.SlotIndex))
                CountInDelegate = Found->Item.Count;
            SumInDelegate = SumCounts(*Container, 2000);
        });

        const bool bApplied = Container->ApplySlotChange(MakeSlot(EP1ContainerType::InventoryConsumable, 0, 2000, 2, 11));

        TestTrue(TEXT("고침: 성공"), bApplied);
        TestEqual(TEXT("고침: 알림 한 번"), Notified, 1);
        TestEqual(TEXT("고침: 알린 칸 번호"), NotifiedSlot.Address.SlotIndex, 0);
        TestEqual(TEXT("고침: 알림 안에서 읽은 칸"), CountInDelegate, 2);
        TestEqual(TEXT("고침: 알림 안에서 읽은 구역 합계"), SumInDelegate, 7);
    }

    // 3) 아이템이 없는 칸 변경은 칸을 비운다. 비운 칸도 유효한 칸으로 남는다.
    {
        UP1ItemContainer* Container = MakeConsumableContainer();
        TestTrue(TEXT("비움: 성공"), Container->ApplySlotChange(MakeSlot(EP1ContainerType::InventoryConsumable, 2)));

        const FP1Slot* Emptied = Container->FindSlot(2);
        if (TestNotNull(TEXT("비움: 칸이 남음"), Emptied))
            TestFalse(TEXT("비움: 아이템 없음"), Emptied->HasItem());
        TestEqual(TEXT("비움: 구역 합계"), SumCounts(*Container, 2000), 3);
    }

    // 4) 유효하지 않은 칸 번호와 다른 컨테이너의 칸 변경은 바꾸지도 알리지도 않는다.
    {
        UP1ItemContainer* Container = MakeConsumableContainer();

        int32 Notified = 0;
        Container->OnSlotChanged.AddLambda([&Notified](const FP1Slot&) { ++Notified; });

        TestFalse(TEXT("범위 밖: 칸 수와 같은 번호"), Container->ApplySlotChange(MakeSlot(EP1ContainerType::InventoryConsumable, 3, 2000, 1, 13)));
        TestFalse(TEXT("범위 밖: 음수 번호"), Container->ApplySlotChange(MakeSlot(EP1ContainerType::InventoryConsumable, -1, 2000, 1, 13)));
        TestFalse(TEXT("다른 컨테이너"), Container->ApplySlotChange(MakeSlot(EP1ContainerType::InventoryGear, 1, 1000, 1, 14)));

        TestEqual(TEXT("범위 밖: 알림 없음"), Notified, 0);
        TestEqual(TEXT("범위 밖: 칸 수 그대로"), Container->GetSlots().Num(), 3);
        TestEqual(TEXT("범위 밖: 구역 합계 그대로"), SumCounts(*Container, 2000), 8);
    }

    // 5) 착용 장비에는 0번 칸이 없다. 칸 번호는 받은 칸들이 정한다.
    {
        UP1ItemContainer* Container = NewObject<UP1ItemContainer>(GetTransientPackage());
        Container->Init(EP1ContainerType::Equipped);
        Container->ResetSlots({ MakeSlot(EP1ContainerType::Equipped, 1), MakeSlot(EP1ContainerType::Equipped, 6) });

        TestFalse(TEXT("착용 장비 0번 칸"), Container->ApplySlotChange(MakeSlot(EP1ContainerType::Equipped, 0, 1000, 1, 31)));
        TestTrue(TEXT("착용 장비 6번 칸"), Container->ApplySlotChange(MakeSlot(EP1ContainerType::Equipped, 6, 1005, 1, 32)));
    }

    // 6) 칸 전체를 바꾸거나 비우면 바꾼 뒤에 전체 재설정을 알린다.
    {
        UP1ItemContainer* Container = MakeConsumableContainer();

        int32 Resets = 0;
        int32 SlotsInDelegate = -1;
        int32 SlotChanges = 0;
        Container->OnReset.AddLambda([&](const UP1ItemContainer& Reset)
        {
            ++Resets;
            SlotsInDelegate = Reset.GetSlots().Num();
        });
        Container->OnSlotChanged.AddLambda([&SlotChanges](const FP1Slot&) { ++SlotChanges; });

        Container->ResetSlots({ MakeSlot(EP1ContainerType::InventoryConsumable, 0, 2001, 1, 41) });
        TestEqual(TEXT("재설정: 알림 한 번"), Resets, 1);
        TestEqual(TEXT("재설정: 알림 안에서 읽은 칸 수"), SlotsInDelegate, 1);
        TestEqual(TEXT("재설정: 칸 변경 알림 없음"), SlotChanges, 0);
        TestNull(TEXT("재설정: 옛 칸 번호가 남지 않음"), Container->FindSlot(2));

        Container->Clear();
        TestEqual(TEXT("비우기: 알림"), Resets, 2);
        TestEqual(TEXT("비우기: 알림 안에서 읽은 칸 수"), SlotsInDelegate, 0);
        TestNull(TEXT("비우기: 칸이 남지 않음"), Container->FindSlot(0));
        TestFalse(TEXT("비우기: 이후 칸 변경을 받지 않음"), Container->ApplySlotChange(MakeSlot(EP1ContainerType::InventoryConsumable, 0, 2000, 1, 42)));
    }

    return true;
}

#endif
