// 클라이언트 퀵슬롯 사본이 고친 뒤에 알리는 규칙과, 받지 않은 번호와 모르는 대상을 버리는 규칙을 고정한다.
//
// 왜 이것인가: 퀵슬롯의 개수는 서버가 입장 패킷으로 알려 준다. 받지 않은 번호의 변경을 넣으면 화면에 없는 퀵슬롯이 생기고,
// 모르는 종류의 대상을 등록된 것으로 들면 그릴 수 없는 퀵슬롯이 된다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.QuickSlots.QuickSlots

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Game/QuickSlots/P1QuickSlotPackets.h"
#include "Game/QuickSlots/P1QuickSlots.h"

#if WITH_DEV_AUTOMATION_TESTS

// 유니티 빌드에서 다른 테스트 파일과 한 번역 단위로 묶여도 도우미 이름이 겹치지 않게 파일 이름의 네임스페이스에 둔다.
namespace P1QuickSlotsTest
{
    FP1QuickSlot MakeQuickSlot(int32 SlotIndex, int32 TemplateId = 0)
    {
        FP1QuickSlot QuickSlot;
        QuickSlot.SlotIndex = SlotIndex;
        if (TemplateId > 0)
            QuickSlot.Ref = FP1QuickSlotRef::ForItem(TemplateId);
        return QuickSlot;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1QuickSlotsTest,
    "P1.QuickSlots.QuickSlots",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1QuickSlotsTest::RunTest(const FString& Parameters)
{
    using namespace P1QuickSlotsTest;

    // 1) 전체를 바꾸면 번호 순서로 들고 알린다.
    UP1QuickSlots* QuickSlots = NewObject<UP1QuickSlots>(GetTransientPackage());
    {
        int32 ResetCount = 0;
        QuickSlots->OnReset.AddLambda([&](const UP1QuickSlots&) { ++ResetCount; });
        QuickSlots->ResetQuickSlots({ MakeQuickSlot(2), MakeQuickSlot(0, 2000), MakeQuickSlot(1) });

        TestEqual(TEXT("전체: 알림"), ResetCount, 1);
        const TArray<FP1QuickSlot>& All = QuickSlots->GetQuickSlots();
        if (TestEqual(TEXT("전체: 개수"), All.Num(), 3))
        {
            TestEqual(TEXT("전체: 첫 번호"), All[0].SlotIndex, 0);
            TestTrue(TEXT("전체: 0번의 대상"), All[0].Ref == FP1QuickSlotRef::ForItem(2000));
            TestEqual(TEXT("전체: 마지막 번호"), All[2].SlotIndex, 2);
        }
        QuickSlots->OnReset.Clear();
    }

    // 2) 하나를 고친 뒤에 알린다. 알림 안에서 읽은 값이 새 값이다.
    {
        FP1QuickSlotRef RefInDelegate;
        QuickSlots->OnChanged.AddLambda([&](const FP1QuickSlot& Changed)
            {
                if (const FP1QuickSlot* Found = QuickSlots->Find(Changed.SlotIndex))
                    RefInDelegate = Found->Ref;
            });

        TestTrue(TEXT("고침: 받아들임"), QuickSlots->ApplyChange(MakeQuickSlot(2, 2001)));
        TestTrue(TEXT("고침: 알림 안에서 읽은 대상"), RefInDelegate == FP1QuickSlotRef::ForItem(2001));

        TestTrue(TEXT("해제: 받아들임"), QuickSlots->ApplyChange(MakeQuickSlot(0)));
        TestTrue(TEXT("해제: 빈 참조"), QuickSlots->Find(0)->Ref.IsEmpty());
        QuickSlots->OnChanged.Clear();
    }

    // 3) 받지 않은 번호의 변경은 바꾸지도 알리지도 않는다.
    {
        int32 ChangedCount = 0;
        QuickSlots->OnChanged.AddLambda([&](const FP1QuickSlot&) { ++ChangedCount; });

        TestFalse(TEXT("범위 밖: 거절"), QuickSlots->ApplyChange(MakeQuickSlot(3, 2000)));
        TestFalse(TEXT("음수: 거절"), QuickSlots->ApplyChange(MakeQuickSlot(-1, 2000)));
        TestEqual(TEXT("범위 밖: 알리지 않음"), ChangedCount, 0);
        TestEqual(TEXT("범위 밖: 개수 그대로"), QuickSlots->GetQuickSlots().Num(), 3);
        QuickSlots->OnChanged.Clear();
    }

    // 4) 패킷의 대상: 참조가 없거나 모르는 종류면 빈 퀵슬롯이다.
    {
        Protocol::QuickSlot Item;
        Item.set_quick_slot_index(4);
        Item.mutable_ref()->set_ref_type(Protocol::QUICK_SLOT_REF_TYPE_ITEM);
        Item.mutable_ref()->set_ref_id(2000);
        const FP1QuickSlot FromItem = P1QuickSlotPackets::FromProto(Item);
        TestEqual(TEXT("패킷: 번호"), FromItem.SlotIndex, 4);
        TestTrue(TEXT("패킷: 아이템 정의"), FromItem.Ref == FP1QuickSlotRef::ForItem(2000));

        Protocol::QuickSlot Empty;
        Empty.set_quick_slot_index(1);
        TestTrue(TEXT("패킷: 참조 없음"), P1QuickSlotPackets::FromProto(Empty).Ref.IsEmpty());

        Protocol::QuickSlot Unknown = Item;
        Unknown.mutable_ref()->set_ref_type(static_cast<Protocol::QuickSlotRefType>(2));
        TestTrue(TEXT("패킷: 모르는 종류"), P1QuickSlotPackets::FromProto(Unknown).Ref.IsEmpty());

        const Protocol::QuickSlotRef Request = P1QuickSlotPackets::ToProto(FP1QuickSlotRef::ForItem(2001));
        TestEqual(TEXT("요청: 종류"), static_cast<int32>(Request.ref_type()), static_cast<int32>(Protocol::QUICK_SLOT_REF_TYPE_ITEM));
        TestEqual(TEXT("요청: 대상"), Request.ref_id(), 2001);
    }

    // 5) 비우면 개수도 0이 된다. 접속이 끊긴 뒤 다른 캐릭터의 배치가 남지 않는다.
    {
        QuickSlots->Clear();
        TestEqual(TEXT("비움: 개수"), QuickSlots->GetQuickSlots().Num(), 0);
    }

    return true;
}

#endif
