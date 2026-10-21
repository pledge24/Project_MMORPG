#include "UI/Screens/P1ShopWidget.h"
#include "UI/Common/P1SlotWidget.h"
#include "Components/PanelWidget.h"
#include "Core/P1GameInstance.h"
#include "Game/Progress/P1ItemActions.h"
#include "Game/Progress/P1MyPlayerData.h"

void UP1ShopWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 진열 칸 우클릭 연결.
    for (UWidget* Child : UGP_Shop->GetAllChildren())
    {
        if (UP1SlotWidget* SlotWidget = Cast<UP1SlotWidget>(Child))
            SlotWidget->OnRightClicked.AddUObject(this, &UP1ShopWidget::HandleSlotRightClicked);
    }
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
