// 아이템 요청 창구가 칸 주소에 대한 의도와 구매를 받아 무엇을 보낼지, 언제 보내지 않을지 정하는 판정을 고정한다.
//
// 왜 이것인가: 판매·사용·착용·해제·구매의 판정과 응답 대기 플래그가 인벤토리 창, 상태 창, 상점 창에 흩어져 있었고,
// 위젯이 패킷을 직접 보냈다. 이제 위젯은 의도만 넘기고 판정과 응답 대기와 송신은 이 창구가 맡는다.
// 데이터는 서버 응답으로만 바뀌므로 요청을 보내도 칸과 골드가 그대로여야 한다.
//
// 기대값은 판정을 다시 계산하지 않고 규칙과 기획 데이터에서 적었다. 두건(1000)과 흑기사의 검(1011, 요구 레벨 10)은
// 팔 수 있고, 초보자의 검(1005)은 팔 수 없다. HP 물약(2000)은 150골드이고 재사용 대기가 10초다. 흑기사의 검은 2000골드다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Progress.ItemActions

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Game/Items/P1ItemContainer.h"
#include "Game/Progress/P1ItemActions.h"
#include "Game/Progress/P1MyPlayerData.h"

#if WITH_DEV_AUTOMATION_TESTS

// 유니티 빌드에서 다른 테스트 파일과 한 번역 단위로 묶여도 도우미 이름이 겹치지 않게 파일 이름의 네임스페이스에 둔다.
namespace P1ItemActionsTest
{
    using EKind = UP1ItemActions::ERequestKind;
    using EResult = UP1ItemActions::EResult;

    constexpr uint64 MY_PLAYER_ID = 100;
    constexpr uint64 OTHER_PLAYER_ID = 200;
    constexpr int32 MY_LEVEL = 5;

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

    /**
     * 골드 1000. 장비 구역 0번 두건(uid 11), 1번 흑기사의 검(uid 12), 2번 초보자의 검(uid 13), 3번 빈 칸.
     * 소모품 구역 0번 HP 물약 3개(uid 21), 1번 빈 칸. 기타 구역 0번 기타 아이템(uid 31). 착용 장비 1~6번 중 6번에 초보자의 검(uid 41).
     */
    Protocol::S_ENTER_GAME MakeEnter()
    {
        Protocol::S_ENTER_GAME Pkt;
        Pkt.set_success(true);
        Pkt.mutable_player()->set_entity_id(MY_PLAYER_ID);
        Pkt.mutable_possession()->set_gold(1000);

        Protocol::ContainerSlots* Gear = Pkt.mutable_possession()->add_containers();
        Gear->set_container(Protocol::CONTAINER_TYPE_INVENTORY_GEAR);
        AddSlot(Gear, 0, 1000, 1, 11);
        AddSlot(Gear, 1, 1011, 1, 12);
        AddSlot(Gear, 2, 1005, 1, 13);
        AddSlot(Gear, 3);

        Protocol::ContainerSlots* Consumable = Pkt.mutable_possession()->add_containers();
        Consumable->set_container(Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE);
        AddSlot(Consumable, 0, 2000, 3, 21);
        AddSlot(Consumable, 1);

        Protocol::ContainerSlots* Misc = Pkt.mutable_possession()->add_containers();
        Misc->set_container(Protocol::CONTAINER_TYPE_INVENTORY_MISC);
        AddSlot(Misc, 0, 4000, 1, 31);

        Protocol::ContainerSlots* Equipped = Pkt.mutable_possession()->add_containers();
        Equipped->set_container(Protocol::CONTAINER_TYPE_EQUIPPED);
        for (int32 GearType = 1; GearType <= 6; ++GearType)
            AddSlot(Equipped, GearType, GearType == 6 ? 1005 : 0, 1, GearType == 6 ? 41 : 0);

        return Pkt;
    }

    FP1SlotAddress Address(EP1ContainerType Container, int32 SlotIndex)
    {
        FP1SlotAddress Result;
        Result.Container = Container;
        Result.SlotIndex = SlotIndex;
        return Result;
    }

    /** 레벨 5로 입장한 내 플레이어와, 보낸 요청을 기록하는 창구. */
    struct FFixture
    {
        UP1MyPlayerData* Data = nullptr;
        UP1ItemActions* Actions = nullptr;
        TArray<UP1ItemActions::FRequest> Sent;

        FFixture()
        {
            // 게임 인스턴스 서브시스템은 게임 인스턴스 안에서만 만들 수 있다. 초기화하지 않은 빈 인스턴스면 된다.
            UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
            Data = NewObject<UP1MyPlayerData>(GameInstance);
            Data->InitMyPlayerData(MakeEnter());
            Data->ApplyLevel(MY_LEVEL);

            Actions = Data->GetItemActions();
            if (Actions)
                Actions->SetRequestSender([this](const UP1ItemActions::FRequest& Request) { Sent.Add(Request); });
        }
    };

    Protocol::S_USE_ITEM MakeUseResponse(uint64 EntityId, bool bSuccess)
    {
        Protocol::S_USE_ITEM Pkt;
        Pkt.set_entity_id(EntityId);
        Pkt.set_success(bSuccess);
        if (bSuccess)
        {
            // 소모품 구역 0번의 물약이 3개에서 2개로 준다.
            Protocol::SlotChange* Change = Pkt.add_changed_slots();
            Change->mutable_address()->set_container(Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE);
            Change->mutable_address()->set_slot_index(0);
            Change->mutable_item()->set_template_id(2000);
            Change->mutable_item()->set_count(2);
            Change->mutable_item()->set_item_uid(21);
        }
        return Pkt;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1ItemActionsTest,
    "P1.Progress.ItemActions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1ItemActionsTest::RunTest(const FString& Parameters)
{
    using namespace P1ItemActionsTest;

    constexpr EP1ContainerType GEAR = EP1ContainerType::InventoryGear;
    constexpr EP1ContainerType CONSUMABLE = EP1ContainerType::InventoryConsumable;
    constexpr EP1ContainerType MISC = EP1ContainerType::InventoryMisc;
    constexpr EP1ContainerType EQUIPPED = EP1ContainerType::Equipped;

    // 1) 소모품을 쓰면 사용 요청, 장비를 쓰면 착용 요청이 칸 주소와 칸의 아이템 uid로 나간다.
    {
        FFixture F;
        if (TestNotNull(TEXT("요청 창구"), F.Actions) == false)
            return false;

        TestTrue(TEXT("물약 사용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(CONSUMABLE, 0)) == EResult::Sent);
        F.Actions->HandleResponse(EKind::Use);
        TestTrue(TEXT("두건 착용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(GEAR, 0)) == EResult::Sent);

        if (TestEqual(TEXT("보낸 요청 수"), F.Sent.Num(), 2))
        {
            TestTrue(TEXT("물약: 사용 요청"), F.Sent[0].Kind == EKind::Use);
            TestTrue(TEXT("물약: 칸 주소"), F.Sent[0].Address == Address(CONSUMABLE, 0));
            TestEqual(TEXT("물약: uid"), F.Sent[0].ItemUid, 21LL);
            TestTrue(TEXT("두건: 착용 요청"), F.Sent[1].Kind == EKind::Equip);
            TestTrue(TEXT("두건: 칸 주소"), F.Sent[1].Address == Address(GEAR, 0));
            TestEqual(TEXT("두건: uid"), F.Sent[1].ItemUid, 11LL);
        }

        // 낙관적 갱신을 하지 않는다.
        const FP1Slot* Potion = F.Data->GetContainer(CONSUMABLE)->FindSlot(0);
        if (TestNotNull(TEXT("보낸 뒤 물약 칸"), Potion))
            TestEqual(TEXT("보낸 뒤 물약 개수는 그대로"), Potion->Item.Count, 3);
    }

    // 2) 요구 레벨이 모자라거나, 쓸 수 없는 아이템이거나, 빈 칸이면 보내지 않는다.
    {
        FFixture F;
        TestTrue(TEXT("Lv10 검을 Lv5가 착용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(GEAR, 1)) == EResult::LevelTooLow);
        TestTrue(TEXT("기타 아이템 사용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(MISC, 0)) == EResult::Unusable);
        TestTrue(TEXT("빈 칸 사용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(GEAR, 3)) == EResult::Empty);
        TestTrue(TEXT("없는 칸 번호 사용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(GEAR, 99)) == EResult::Empty);
        TestTrue(TEXT("컨테이너가 아닌 주소"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(EP1ContainerType::None, 0)) == EResult::Empty);
        TestTrue(TEXT("착용 장비 칸을 사용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(EQUIPPED, 6)) == EResult::Unusable);
        TestTrue(TEXT("의도 없음"), F.Actions->RequestForSlot(EP1ItemIntent::None, Address(CONSUMABLE, 0)) == EResult::Unusable);
        TestEqual(TEXT("보낸 요청 없음"), F.Sent.Num(), 0);
    }

    // 3) 인벤토리 칸의 사용, 착용, 판매는 응답 대기 하나를 함께 쓴다. 해제와 구매는 그 대기에 막히지 않는다.
    //    다른 플레이어의 응답으로는 풀리지 않고, 실패 응답이어도 내 응답이면 풀린다.
    {
        FFixture F;
        F.Actions->SetShopOpen(true);
        TestTrue(TEXT("첫 사용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(CONSUMABLE, 0)) == EResult::Sent);
        TestTrue(TEXT("응답 전 두 번째 사용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(CONSUMABLE, 0)) == EResult::Pending);
        TestTrue(TEXT("사용 대기 중 착용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(GEAR, 0)) == EResult::Pending);
        TestTrue(TEXT("사용 대기 중 판매"), F.Actions->RequestForSlot(EP1ItemIntent::Sell, Address(GEAR, 0)) == EResult::Pending);
        TestTrue(TEXT("사용 대기 중 해제"), F.Actions->RequestForSlot(EP1ItemIntent::Unequip, Address(EQUIPPED, 6)) == EResult::Sent);
        TestTrue(TEXT("사용 대기 중 구매"), F.Actions->RequestBuy(2000) == EResult::Sent);

        F.Data->HandleUseItem(MakeUseResponse(OTHER_PLAYER_ID, true));
        TestTrue(TEXT("다른 플레이어의 응답 뒤 사용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(CONSUMABLE, 0)) == EResult::Pending);

        F.Data->HandleUseItem(MakeUseResponse(MY_PLAYER_ID, false));
        TestTrue(TEXT("실패 응답 뒤 사용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(CONSUMABLE, 0)) == EResult::Sent);
        TestEqual(TEXT("보낸 요청 수"), F.Sent.Num(), 4);
    }

    // 4) 재사용 대기 중인 소모품은 쓰지 않는다. 대기는 성공한 사용 응답으로 시작한다.
    {
        FFixture F;
        TestTrue(TEXT("사용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(CONSUMABLE, 0)) == EResult::Sent);
        F.Data->HandleUseItem(MakeUseResponse(MY_PLAYER_ID, true));
        TestTrue(TEXT("대기 중 사용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(CONSUMABLE, 0)) == EResult::CoolingDown);
        TestEqual(TEXT("보낸 요청 수"), F.Sent.Num(), 1);
    }

    // 5) 착용 장비 칸을 벗는다. 빈 부위와 인벤토리 칸은 벗지 않는다.
    {
        FFixture F;
        TestTrue(TEXT("빈 부위 해제"), F.Actions->RequestForSlot(EP1ItemIntent::Unequip, Address(EQUIPPED, 1)) == EResult::Empty);
        TestTrue(TEXT("인벤토리 칸 해제"), F.Actions->RequestForSlot(EP1ItemIntent::Unequip, Address(GEAR, 0)) == EResult::Unusable);
        TestTrue(TEXT("무기 해제"), F.Actions->RequestForSlot(EP1ItemIntent::Unequip, Address(EQUIPPED, 6)) == EResult::Sent);
        TestTrue(TEXT("응답 전 무기 해제"), F.Actions->RequestForSlot(EP1ItemIntent::Unequip, Address(EQUIPPED, 6)) == EResult::Pending);

        if (TestEqual(TEXT("보낸 요청 수"), F.Sent.Num(), 1))
        {
            TestTrue(TEXT("해제 요청"), F.Sent[0].Kind == EKind::Unequip);
            TestTrue(TEXT("해제: 칸 주소"), F.Sent[0].Address == Address(EQUIPPED, 6));
            TestEqual(TEXT("해제: uid"), F.Sent[0].ItemUid, 41LL);
        }

        Protocol::S_UNEQUIP_GEAR Response;
        Response.set_entity_id(MY_PLAYER_ID);
        Response.set_success(false);
        F.Data->HandleUnequipGear(Response);
        TestTrue(TEXT("응답 뒤 무기 해제"), F.Actions->RequestForSlot(EP1ItemIntent::Unequip, Address(EQUIPPED, 6)) == EResult::Sent);
    }

    // 6) 판매는 상점 창이 열려 있을 때 인벤토리 칸의 팔 수 있는 아이템을 하나씩 판다.
    {
        FFixture F;
        TestTrue(TEXT("상점이 닫힌 채 판매"), F.Actions->RequestForSlot(EP1ItemIntent::Sell, Address(GEAR, 0)) == EResult::ShopClosed);

        F.Actions->SetShopOpen(true);
        TestTrue(TEXT("빈 칸 판매"), F.Actions->RequestForSlot(EP1ItemIntent::Sell, Address(GEAR, 3)) == EResult::Empty);
        TestTrue(TEXT("팔 수 없는 검 판매"), F.Actions->RequestForSlot(EP1ItemIntent::Sell, Address(GEAR, 2)) == EResult::NotSellable);
        TestTrue(TEXT("착용 장비 판매"), F.Actions->RequestForSlot(EP1ItemIntent::Sell, Address(EQUIPPED, 6)) == EResult::Unusable);
        TestTrue(TEXT("두건 판매"), F.Actions->RequestForSlot(EP1ItemIntent::Sell, Address(GEAR, 0)) == EResult::Sent);
        TestTrue(TEXT("응답 전 물약 판매"), F.Actions->RequestForSlot(EP1ItemIntent::Sell, Address(CONSUMABLE, 0)) == EResult::Pending);

        if (TestEqual(TEXT("보낸 요청 수"), F.Sent.Num(), 1))
        {
            TestTrue(TEXT("판매 요청"), F.Sent[0].Kind == EKind::Sell);
            TestTrue(TEXT("판매: 칸 주소"), F.Sent[0].Address == Address(GEAR, 0));
            TestEqual(TEXT("판매: uid"), F.Sent[0].ItemUid, 11LL);
            TestEqual(TEXT("판매: 개수"), F.Sent[0].Count, 1);
        }

        TestNotNull(TEXT("보낸 뒤 두건 칸은 그대로"), F.Data->GetContainer(GEAR)->FindSlot(0));
        TestEqual(TEXT("보낸 뒤 골드는 그대로"), F.Data->GetGold(), 1000LL);

        Protocol::S_SELL_ITEM Response;
        Response.set_success(false);
        F.Data->HandleSellItem(Response);
        TestTrue(TEXT("응답 뒤 물약 판매"), F.Actions->RequestForSlot(EP1ItemIntent::Sell, Address(CONSUMABLE, 0)) == EResult::Sent);

        F.Actions->SetShopOpen(false);
        F.Data->HandleSellItem(Response);
        TestTrue(TEXT("상점을 닫은 뒤 판매"), F.Actions->RequestForSlot(EP1ItemIntent::Sell, Address(GEAR, 0)) == EResult::ShopClosed);
    }

    // 7) 구매는 골드가 가격 이상일 때 템플릿 하나를 산다.
    {
        FFixture F;
        TestTrue(TEXT("2000골드 검을 1000골드로 구매"), F.Actions->RequestBuy(1011) == EResult::NotEnoughGold);
        TestTrue(TEXT("정의가 없는 템플릿 구매"), F.Actions->RequestBuy(0) == EResult::Empty);
        TestTrue(TEXT("물약 구매"), F.Actions->RequestBuy(2000) == EResult::Sent);
        TestTrue(TEXT("응답 전 물약 구매"), F.Actions->RequestBuy(2000) == EResult::Pending);

        if (TestEqual(TEXT("보낸 요청 수"), F.Sent.Num(), 1))
        {
            TestTrue(TEXT("구매 요청"), F.Sent[0].Kind == EKind::Buy);
            TestEqual(TEXT("구매: 템플릿"), F.Sent[0].TemplateId, 2000);
            TestEqual(TEXT("구매: 개수"), F.Sent[0].Count, 1);
        }
        TestEqual(TEXT("보낸 뒤 골드는 그대로"), F.Data->GetGold(), 1000LL);

        Protocol::S_BUY_ITEM Response;
        Response.set_success(false);
        F.Data->HandleBuyItem(Response);
        TestTrue(TEXT("응답 뒤 물약 구매"), F.Actions->RequestBuy(2000) == EResult::Sent);
    }

    // 8) 새로 입장하면 응답 대기가 풀린다. 끊기기 전에 보낸 요청의 응답은 오지 않는다.
    {
        FFixture F;
        TestTrue(TEXT("입장 전 사용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(CONSUMABLE, 0)) == EResult::Sent);
        F.Data->InitMyPlayerData(MakeEnter());
        F.Data->ApplyLevel(MY_LEVEL);
        TestTrue(TEXT("다시 입장한 뒤 사용"), F.Actions->RequestForSlot(EP1ItemIntent::Use, Address(CONSUMABLE, 0)) == EResult::Sent);
    }

    return true;
}

#endif
