// 아이템과 칸의 패킷 변환을 고정한다.
//
// 왜 이것인가: 클라이언트가 칸 상태를 Protocol 타입 대신 전용 타입으로 들게 되면서, 패킷에 실린 값이 이 변환을
// 거쳐야만 상태가 된다. 칸에 필드를 더하고 변환을 잊으면 빌드는 통과하고 값만 사라진다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Items.ItemPackets

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Game/Items/P1ItemPackets.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
    void SetItem(Protocol::Item* Item, int64 ItemUid, int32 TemplateId, int32 Count)
    {
        Item->set_item_uid(ItemUid);
        Item->set_template_id(TemplateId);
        Item->set_count(Count);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1ItemPacketsTest,
    "P1.Items.ItemPackets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1ItemPacketsTest::RunTest(const FString& Parameters)
{
    // 1) 컨테이너 종류는 값마다 짝이 맞고, 모르는 값은 None이 된다.
    {
        const TPair<Protocol::ContainerType, EP1ContainerType> Pairs[] = {
            { Protocol::CONTAINER_TYPE_NONE, EP1ContainerType::None },
            { Protocol::CONTAINER_TYPE_INVENTORY_GEAR, EP1ContainerType::InventoryGear },
            { Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE, EP1ContainerType::InventoryConsumable },
            { Protocol::CONTAINER_TYPE_INVENTORY_MISC, EP1ContainerType::InventoryMisc },
            { Protocol::CONTAINER_TYPE_EQUIPPED, EP1ContainerType::Equipped },
        };
        for (const auto& Pair : Pairs)
        {
            TestTrue(FString::Printf(TEXT("패킷에서 컨테이너 %d"), static_cast<int32>(Pair.Key)), P1ItemPackets::FromProto(Pair.Key) == Pair.Value);
            TestTrue(FString::Printf(TEXT("패킷으로 컨테이너 %d"), static_cast<int32>(Pair.Key)), P1ItemPackets::ToProto(Pair.Value) == Pair.Key);
        }

        TestTrue(TEXT("모르는 컨테이너 값"), P1ItemPackets::FromProto(static_cast<Protocol::ContainerType>(99)) == EP1ContainerType::None);
    }

    // 2) 칸 주소는 왕복한다.
    {
        FP1SlotAddress Address;
        Address.Container = EP1ContainerType::Equipped;
        Address.SlotIndex = 6;

        const Protocol::SlotAddress Proto = P1ItemPackets::ToProto(Address);
        TestEqual(TEXT("주소: 패킷의 컨테이너"), static_cast<int32>(Proto.container()), static_cast<int32>(Protocol::CONTAINER_TYPE_EQUIPPED));
        TestEqual(TEXT("주소: 패킷의 칸 번호"), Proto.slot_index(), 6);
        TestTrue(TEXT("주소: 왕복"), P1ItemPackets::FromProto(Proto) == Address);
    }

    // 3) 칸 변경은 주소와 아이템의 uid, 템플릿, 개수를 싣는다.
    {
        Protocol::SlotChange Change;
        Change.mutable_address()->set_container(Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE);
        Change.mutable_address()->set_slot_index(4);
        SetItem(Change.mutable_item(), 7001, 2000, 3);

        const FP1Slot Slot_ = P1ItemPackets::FromProto(Change);
        TestTrue(TEXT("칸 변경: 컨테이너"), Slot_.Address.Container == EP1ContainerType::InventoryConsumable);
        TestEqual(TEXT("칸 변경: 칸 번호"), Slot_.Address.SlotIndex, 4);
        TestTrue(TEXT("칸 변경: 아이템 있음"), Slot_.HasItem());
        TestEqual(TEXT("칸 변경: uid"), Slot_.Item.ItemUid, static_cast<int64>(7001));
        TestEqual(TEXT("칸 변경: 템플릿"), Slot_.Item.TemplateId, 2000);
        TestEqual(TEXT("칸 변경: 개수"), Slot_.Item.Count, 3);
    }

    // 4) 아이템이 없는 칸 변경은 주소만 있는 빈 칸이 된다.
    {
        Protocol::SlotChange Change;
        Change.mutable_address()->set_container(Protocol::CONTAINER_TYPE_INVENTORY_GEAR);
        Change.mutable_address()->set_slot_index(2);

        const FP1Slot Slot_ = P1ItemPackets::FromProto(Change);
        TestTrue(TEXT("빈 칸 변경: 컨테이너"), Slot_.Address.Container == EP1ContainerType::InventoryGear);
        TestEqual(TEXT("빈 칸 변경: 칸 번호"), Slot_.Address.SlotIndex, 2);
        TestFalse(TEXT("빈 칸 변경: 아이템 없음"), Slot_.HasItem());
        TestTrue(TEXT("빈 칸 변경: 아이템이 기본값"), Slot_.Item == FP1ItemInstance());
    }

    // 5) 입장 패킷의 컨테이너 하나는 실린 순서대로 칸이 되고, 주소의 컨테이너를 그 컨테이너로 채운다.
    {
        Protocol::ContainerSlots ContainerSlots;
        ContainerSlots.set_container(Protocol::CONTAINER_TYPE_EQUIPPED);

        Protocol::Slot* Helmet = ContainerSlots.add_slots();
        Helmet->set_slot_index(1);
        SetItem(Helmet->mutable_item(), 8001, 1000, 1);

        Protocol::Slot* Weapon = ContainerSlots.add_slots();
        Weapon->set_slot_index(6);

        const TArray<FP1Slot> Slots = P1ItemPackets::FromProto(ContainerSlots);
        if (TestEqual(TEXT("컨테이너: 칸 수"), Slots.Num(), 2))
        {
            TestTrue(TEXT("컨테이너: 첫 칸의 컨테이너"), Slots[0].Address.Container == EP1ContainerType::Equipped);
            TestEqual(TEXT("컨테이너: 첫 칸 번호"), Slots[0].Address.SlotIndex, 1);
            TestEqual(TEXT("컨테이너: 첫 칸 uid"), Slots[0].Item.ItemUid, static_cast<int64>(8001));
            TestEqual(TEXT("컨테이너: 첫 칸 템플릿"), Slots[0].Item.TemplateId, 1000);
            TestEqual(TEXT("컨테이너: 첫 칸 개수"), Slots[0].Item.Count, 1);

            TestTrue(TEXT("컨테이너: 둘째 칸의 컨테이너"), Slots[1].Address.Container == EP1ContainerType::Equipped);
            TestEqual(TEXT("컨테이너: 둘째 칸 번호"), Slots[1].Address.SlotIndex, 6);
            TestFalse(TEXT("컨테이너: 둘째 칸은 빈 칸"), Slots[1].HasItem());
        }
    }

    return true;
}

#endif
