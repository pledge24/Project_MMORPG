// 내 플레이어 데이터가 입장 패킷과 응답의 칸 변경을 컨테이너별로 나눠 주는 규칙을 고정한다.
//
// 왜 이것인가: 칸 변경은 인벤토리용과 장비용 델리게이트 둘로 나뉘어 퍼졌고, 칸 사본과 위젯이 같은 델리게이트를
// 구독했다. 이제 컨테이너가 자기 칸을 고친 뒤에 자기 델리게이트로 알린다. 장비창은 소모품 변경을 받지 않는다.
// 접속이 끊긴 뒤 다른 캐릭터로 들어가도 이전 캐릭터의 칸이 남지 않아야 한다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Progress.MyPlayerContainers

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Game/Items/P1ItemContainer.h"
#include "Game/Progress/P1MyPlayerData.h"

#if WITH_DEV_AUTOMATION_TESTS

// 유니티 빌드에서 다른 테스트 파일과 한 번역 단위로 묶여도 도우미 이름이 겹치지 않게 파일 이름의 네임스페이스에 둔다.
namespace P1MyPlayerContainersTest
{
    constexpr uint64 MY_PLAYER_ID = 100;

    void AddSlot(Protocol::ContainerSlots* ContainerSlots, int32 SlotIndex, int32 TemplateId = 0, int32 Count = 0, int64 ItemUid = 0)
    {
        Protocol::Slot* Slot_ = ContainerSlots->add_slots();
        Slot_->set_slot_index(SlotIndex);
        if (TemplateId > 0)
        {
            Slot_->mutable_item()->set_template_id(TemplateId);
            Slot_->mutable_item()->set_count(Count);
            Slot_->mutable_item()->set_item_uid(ItemUid);
        }
    }

    void AddChange(google::protobuf::RepeatedPtrField<Protocol::SlotChange>* Changes, Protocol::ContainerType Container, int32 SlotIndex,
        int32 TemplateId = 0, int32 Count = 0, int64 ItemUid = 0)
    {
        Protocol::SlotChange* Change = Changes->Add();
        Change->mutable_address()->set_container(Container);
        Change->mutable_address()->set_slot_index(SlotIndex);
        if (TemplateId > 0)
        {
            Change->mutable_item()->set_template_id(TemplateId);
            Change->mutable_item()->set_count(Count);
            Change->mutable_item()->set_item_uid(ItemUid);
        }
    }

    /** 입장 응답 뒤의 룸 입장. 내 엔티티 번호는 이 응답이 알린다. */
    void EnterRoom(UP1MyPlayerData* Data, uint64 PlayerId)
    {
        Protocol::S_ENTER_ROOM Pkt;
        Pkt.set_result(Protocol::ENTRY_RESULT_SUCCESS);
        Pkt.mutable_my_entity()->set_entity_id(PlayerId);
        Data->HandleEnterRoom(Pkt);
    }

    /**
     * 장비 구역 칸 2개(0번에 투구), 소모품 구역 칸 3개(0번에 물약 3개, 2번에 물약 5개), 기타 구역 칸 1개,
     * 착용 장비 칸 1~6(6번에 무기)을 가진 캐릭터의 입장 패킷.
     */
    Protocol::S_ENTER_GAME MakeFirstCharacterEnter()
    {
        Protocol::S_ENTER_GAME Pkt;
        Pkt.set_result(Protocol::ENTRY_RESULT_SUCCESS);
        Pkt.mutable_possession()->set_gold(1000);

        Protocol::ContainerSlots* Gear = Pkt.mutable_possession()->add_containers();
        Gear->set_container(Protocol::CONTAINER_TYPE_INVENTORY_GEAR);
        AddSlot(Gear, 0, 1000, 1, 11);
        AddSlot(Gear, 1);

        Protocol::ContainerSlots* Consumable = Pkt.mutable_possession()->add_containers();
        Consumable->set_container(Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE);
        AddSlot(Consumable, 0, 2000, 3, 21);
        AddSlot(Consumable, 1);
        AddSlot(Consumable, 2, 2000, 5, 22);

        Protocol::ContainerSlots* Misc = Pkt.mutable_possession()->add_containers();
        Misc->set_container(Protocol::CONTAINER_TYPE_INVENTORY_MISC);
        AddSlot(Misc, 0, 4000, 1, 31);

        Protocol::ContainerSlots* Equipped = Pkt.mutable_possession()->add_containers();
        Equipped->set_container(Protocol::CONTAINER_TYPE_EQUIPPED);
        for (int32 GearType = 1; GearType <= 6; ++GearType)
            AddSlot(Equipped, GearType, GearType == 6 ? 1005 : 0, 1, GearType == 6 ? 41 : 0);

        return Pkt;
    }

    UP1MyPlayerData* MakeData()
    {
        // 게임 인스턴스 서브시스템은 게임 인스턴스 안에서만 만들 수 있다. 초기화하지 않은 빈 인스턴스면 된다.
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        return NewObject<UP1MyPlayerData>(GameInstance);
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

    /** 컨테이너의 칸 변경 알림을 센다. */
    struct FSlotChangeCounter
    {
        int32 Count = 0;

        void Watch(UP1ItemContainer* Container)
        {
            Container->OnSlotChanged.AddLambda([this](const FP1Slot&) { ++Count; });
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1MyPlayerContainersTest,
    "P1.Progress.MyPlayerContainers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1MyPlayerContainersTest::RunTest(const FString& Parameters)
{
    using namespace P1MyPlayerContainersTest;

    // 1) 입장 패킷의 컨테이너가 같은 종류의 컨테이너로 간다.
    {
        UP1MyPlayerData* Data = MakeData();
        Data->InitMyPlayerData(MakeFirstCharacterEnter());
        EnterRoom(Data, MY_PLAYER_ID);

        UP1ItemContainer* Consumable = Data->GetContainer(EP1ContainerType::InventoryConsumable);
        UP1ItemContainer* Equipped = Data->GetContainer(EP1ContainerType::Equipped);
        if (TestNotNull(TEXT("입장: 소모품 구역"), Consumable) && TestNotNull(TEXT("입장: 착용 장비"), Equipped))
        {
            TestEqual(TEXT("입장: 소모품 칸 수"), Consumable->GetSlots().Num(), 3);
            TestEqual(TEXT("입장: 물약 합계"), SumCounts(*Consumable, 2000), 8);
            TestEqual(TEXT("입장: 착용 장비 칸 수"), Equipped->GetSlots().Num(), 6);
            TestNull(TEXT("입장: 착용 장비에 0번 칸 없음"), Equipped->FindSlot(0));

            const FP1Slot* Weapon = Equipped->FindSlot(6);
            if (TestNotNull(TEXT("입장: 무기 칸"), Weapon))
                TestEqual(TEXT("입장: 무기"), Weapon->Item.TemplateId, 1005);
        }

        TestNull(TEXT("컨테이너가 아닌 종류"), Data->GetContainer(EP1ContainerType::None));
    }

    // 2) 응답의 칸 변경은 그 칸의 컨테이너만 알린다. 알림 안에서 읽은 구역 합계가 새 값이다.
    {
        UP1MyPlayerData* Data = MakeData();
        Data->InitMyPlayerData(MakeFirstCharacterEnter());
        EnterRoom(Data, MY_PLAYER_ID);

        UP1ItemContainer* Consumable = Data->GetContainer(EP1ContainerType::InventoryConsumable);
        FSlotChangeCounter GearChanges, EquippedChanges, MiscChanges;
        GearChanges.Watch(Data->GetContainer(EP1ContainerType::InventoryGear));
        EquippedChanges.Watch(Data->GetContainer(EP1ContainerType::Equipped));
        MiscChanges.Watch(Data->GetContainer(EP1ContainerType::InventoryMisc));

        int32 ConsumableChanges = 0;
        int32 SumInDelegate = -1;
        Consumable->OnSlotChanged.AddLambda([&](const FP1Slot&)
        {
            ++ConsumableChanges;
            SumInDelegate = SumCounts(*Consumable, 2000);
        });

        Protocol::S_BUY_ITEM Pkt;
        Pkt.set_result(Protocol::ITEM_RESULT_SUCCESS);
        Pkt.set_gold(900);
        AddChange(Pkt.mutable_changed_slots(), Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE, 1, 2000, 4, 23);
        Data->HandleBuyItem(Pkt);

        TestEqual(TEXT("구매: 소모품 알림"), ConsumableChanges, 1);
        TestEqual(TEXT("구매: 알림 안에서 읽은 물약 합계"), SumInDelegate, 12);
        TestEqual(TEXT("구매: 장비창은 소모품 변경을 받지 않음"), EquippedChanges.Count, 0);
        TestEqual(TEXT("구매: 장비 구역 알림 없음"), GearChanges.Count, 0);
        TestEqual(TEXT("구매: 기타 구역 알림 없음"), MiscChanges.Count, 0);
        TestEqual(TEXT("구매: 골드"), Data->GetGold(), static_cast<int64>(900));
    }

    // 3) 착용 응답은 장비 구역과 착용 장비를 한 번씩 알린다.
    {
        UP1MyPlayerData* Data = MakeData();
        Data->InitMyPlayerData(MakeFirstCharacterEnter());
        EnterRoom(Data, MY_PLAYER_ID);

        FSlotChangeCounter GearChanges, EquippedChanges, ConsumableChanges;
        GearChanges.Watch(Data->GetContainer(EP1ContainerType::InventoryGear));
        EquippedChanges.Watch(Data->GetContainer(EP1ContainerType::Equipped));
        ConsumableChanges.Watch(Data->GetContainer(EP1ContainerType::InventoryConsumable));

        Protocol::S_EQUIP_GEAR Pkt;
        Pkt.set_result(Protocol::ITEM_RESULT_SUCCESS);
        AddChange(Pkt.mutable_changed_slots(), Protocol::CONTAINER_TYPE_INVENTORY_GEAR, 0);
        AddChange(Pkt.mutable_changed_slots(), Protocol::CONTAINER_TYPE_EQUIPPED, 1, 1000, 1, 11);
        Data->HandleEquipGear(Pkt);

        TestEqual(TEXT("착용: 장비 구역 알림"), GearChanges.Count, 1);
        TestEqual(TEXT("착용: 착용 장비 알림"), EquippedChanges.Count, 1);
        TestEqual(TEXT("착용: 소모품 알림 없음"), ConsumableChanges.Count, 0);

        const FP1Slot* Helmet = Data->GetContainer(EP1ContainerType::Equipped)->FindSlot(1);
        if (TestNotNull(TEXT("착용: 투구 칸"), Helmet))
            TestEqual(TEXT("착용: 투구 uid"), Helmet->Item.ItemUid, static_cast<int64>(11));
        TestFalse(TEXT("착용: 장비 구역 0번 칸이 빔"), Data->GetContainer(EP1ContainerType::InventoryGear)->FindSlot(0)->HasItem());
    }

    // 4) 컨테이너가 아니거나 없는 칸의 변경은 버리고, 같은 응답의 다른 칸 변경은 반영한다.
    {
        UP1MyPlayerData* Data = MakeData();
        Data->InitMyPlayerData(MakeFirstCharacterEnter());
        EnterRoom(Data, MY_PLAYER_ID);

        FSlotChangeCounter ConsumableChanges;
        ConsumableChanges.Watch(Data->GetContainer(EP1ContainerType::InventoryConsumable));

        Protocol::S_SELL_ITEM Pkt;
        Pkt.set_result(Protocol::ITEM_RESULT_SUCCESS);
        Pkt.set_gold(1100);
        AddChange(Pkt.mutable_changed_slots(), Protocol::CONTAINER_TYPE_NONE, 0, 2000, 1, 24);
        AddChange(Pkt.mutable_changed_slots(), Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE, 3, 2000, 1, 25);
        AddChange(Pkt.mutable_changed_slots(), Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE, 2, 2000, 4, 22);
        Data->HandleSellItem(Pkt);

        TestEqual(TEXT("버림: 유효한 칸 변경만 알림"), ConsumableChanges.Count, 1);
        TestEqual(TEXT("버림: 소모품 칸 수 그대로"), Data->GetContainer(EP1ContainerType::InventoryConsumable)->GetSlots().Num(), 3);
        TestEqual(TEXT("버림: 물약 합계"), SumCounts(*Data->GetContainer(EP1ContainerType::InventoryConsumable), 2000), 7);
    }

    // 5) 접속이 끊기면 모든 컨테이너를 비운다. 다른 캐릭터로 들어가면 그 캐릭터의 칸만 있다.
    {
        UP1MyPlayerData* Data = MakeData();
        Data->InitMyPlayerData(MakeFirstCharacterEnter());
        EnterRoom(Data, MY_PLAYER_ID);

        int32 Resets = 0;
        for (EP1ContainerType ContainerType : { EP1ContainerType::InventoryGear, EP1ContainerType::InventoryConsumable, EP1ContainerType::InventoryMisc, EP1ContainerType::Equipped })
            Data->GetContainer(ContainerType)->OnReset.AddLambda([&Resets](const UP1ItemContainer&) { ++Resets; });

        Data->ClearContainers();
        TestEqual(TEXT("접속 종료: 컨테이너마다 재설정 알림"), Resets, 4);
        TestEqual(TEXT("접속 종료: 소모품 칸 없음"), Data->GetContainer(EP1ContainerType::InventoryConsumable)->GetSlots().Num(), 0);
        TestEqual(TEXT("접속 종료: 착용 장비 칸 없음"), Data->GetContainer(EP1ContainerType::Equipped)->GetSlots().Num(), 0);

        // 둘째 캐릭터는 소모품 칸 하나만 가지고, 입장 패킷에 기타 구역이 없다.
        Protocol::S_ENTER_GAME Second;
        Second.set_result(Protocol::ENTRY_RESULT_SUCCESS);
        Protocol::ContainerSlots* Consumable = Second.mutable_possession()->add_containers();
        Consumable->set_container(Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE);
        AddSlot(Consumable, 0, 2001, 2, 51);
        Data->InitMyPlayerData(Second);

        const UP1ItemContainer* SecondConsumable = Data->GetContainer(EP1ContainerType::InventoryConsumable);
        TestEqual(TEXT("다른 캐릭터: 소모품 칸 수"), SecondConsumable->GetSlots().Num(), 1);
        TestEqual(TEXT("다른 캐릭터: 이전 캐릭터의 물약 없음"), SumCounts(*SecondConsumable, 2000), 0);
        TestEqual(TEXT("다른 캐릭터: 기타 구역 칸 없음"), Data->GetContainer(EP1ContainerType::InventoryMisc)->GetSlots().Num(), 0);
        TestEqual(TEXT("다른 캐릭터: 착용 장비 칸 없음"), Data->GetContainer(EP1ContainerType::Equipped)->GetSlots().Num(), 0);
    }

    // 6) 접속 종료 없이 다른 캐릭터로 들어가도 입장 패킷에 없는 컨테이너는 빈다.
    {
        UP1MyPlayerData* Data = MakeData();
        Data->InitMyPlayerData(MakeFirstCharacterEnter());
        EnterRoom(Data, MY_PLAYER_ID);

        Protocol::S_ENTER_GAME Second;
        Second.set_result(Protocol::ENTRY_RESULT_SUCCESS);
        Data->InitMyPlayerData(Second);

        TestEqual(TEXT("재입장: 소모품 칸 없음"), Data->GetContainer(EP1ContainerType::InventoryConsumable)->GetSlots().Num(), 0);
        TestEqual(TEXT("재입장: 착용 장비 칸 없음"), Data->GetContainer(EP1ContainerType::Equipped)->GetSlots().Num(), 0);
    }

    return true;
}

#endif
