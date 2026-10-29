#include "UI/Screens/P1ShopWidget.h"
#include "UI/Common/P1SlotWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Core/P1GameInstance.h"
#include "Core/P1InGamePlayerController.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/Generated/P1ShopRow.h"
#include "Game/Interaction/P1NpcInteractionComponent.h"
#include "Game/Progress/P1ItemActions.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "UI/P1ScreenSubsystem.h"
#include "Utils/LogCategory.h"

void UP1ShopWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 상점이 열린 동안 키를 받으려면 포커스를 받을 수 있어야 한다.
    SetIsFocusable(true);
}

void UP1ShopWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 진열 칸 우클릭 연결.
    for (UWidget* Child : UGP_Shop->GetAllChildren())
    {
        if (UP1SlotWidget* SlotWidget = Cast<UP1SlotWidget>(Child))
            SlotWidget->OnRightClicked.AddUObject(this, &UP1ShopWidget::HandleSlotRightClicked);
    }

    if (Close_btn)
        Close_btn->OnClicked.AddUniqueDynamic(this, &UP1ShopWidget::HandleCloseClicked);
}

void UP1ShopWidget::SetShop(int32 ShopId)
{
    const FP1ShopRow* Shop = UP1GameDataSettings::FindShopRow(ShopId);
    if (Shop == nullptr)
        UE_LOG(LogP1UI, Warning, TEXT("상점 표에 %d번 상점이 없어 진열 칸을 비운다"), ShopId);

    const TArray<int32> Items = Shop ? Shop->Items : TArray<int32>();
    int32 ItemIndex = 0;
    for (UWidget* Child : UGP_Shop->GetAllChildren())
    {
        UP1SlotWidget* SlotWidget = Cast<UP1SlotWidget>(Child);
        if (SlotWidget == nullptr)
            continue;

        FP1SlotDisplay Display;
        if (Items.IsValidIndex(ItemIndex))
            Display.TemplateId = Items[ItemIndex];
        SlotWidget->SetDisplay(Display);
        ++ItemIndex;
    }

    if (Items.Num() > ItemIndex)
        UE_LOG(LogP1UI, Warning, TEXT("상점 %d의 판매 목록 %d개가 진열 칸 %d개보다 많다"), ShopId, Items.Num(), ItemIndex);
}

FReply UP1ShopWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // 상점이 열려 있으면 입력이 플레이어 컨트롤러에 가지 않는다. 메뉴 키와 채팅 키는 이 창이 처리한다.
    const AP1InGamePlayerController* PlayerController = GetP1PlayerController<AP1InGamePlayerController>();
    if (PlayerController && PlayerController->IsGameMenuKey(InKeyEvent.GetKey()))
    {
        CloseShop();
        return FReply::Handled();
    }

    if (PlayerController && PlayerController->IsOpenChatKey(InKeyEvent.GetKey()))
    {
        if (UP1ScreenSubsystem* Screens = ULocalPlayer::GetSubsystem<UP1ScreenSubsystem>(GetOwningLocalPlayer()))
            Screens->OpenChatInput();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UP1ShopWidget::HandleCloseClicked()
{
    CloseShop();
}

void UP1ShopWidget::CloseShop()
{
    if (AP1InGamePlayerController* PlayerController = GetP1PlayerController<AP1InGamePlayerController>())
        PlayerController->GetNpcInteraction()->CloseShop();
}

void UP1ShopWidget::HandleSlotRightClicked(UP1SlotWidget* SlotWidget)
{
    UP1GameInstance* GameInstance = GetP1GameInstance();
    UP1MyPlayerData* MyPlayerData = GameInstance ? GameInstance->GetSubsystem<UP1MyPlayerData>() : nullptr;
    if (MyPlayerData == nullptr)
        return;

    // 진열 칸이 그리는 아이템이 파는 아이템이다. 빈 진열 칸이면 요청 창구가 거른다.
    MyPlayerData->GetItemActions()->RequestBuy(SlotWidget->GetDisplay().TemplateId);
}
