#include "UI/Screens/P1InventoryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Core/P1GameInstance.h"
#include "Game/Progress/P1MyPlayerData.h"

void UP1InventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 탭 버튼 연결.
    InventoryTabSwitcher->SetActiveWidgetIndex(0);
    GearTabButton->OnClicked.AddUniqueDynamic(this, &UP1InventoryWidget::ShowGearTab);
    ConsumableTabButton->OnClicked.AddUniqueDynamic(this, &UP1InventoryWidget::ShowConsumableTab);
    MiscTabButton->OnClicked.AddUniqueDynamic(this, &UP1InventoryWidget::ShowMiscTab);

    // 지금 골드로 채우고 이후 바뀌는 골드를 구독.
    if (auto* GameInstance = GetP1GameInstance())
    {
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
        {
            UpdateGold(MyPlayerData->GetGold());
            MyPlayerData->OnGoldChanged.AddUObject(this, &UP1InventoryWidget::UpdateGold);
        }
    }
}

void UP1InventoryWidget::ShowGearTab()
{
    InventoryTabSwitcher->SetActiveWidgetIndex(0);
}

void UP1InventoryWidget::ShowConsumableTab()
{
    InventoryTabSwitcher->SetActiveWidgetIndex(1);
}

void UP1InventoryWidget::ShowMiscTab()
{
    InventoryTabSwitcher->SetActiveWidgetIndex(2);
}

void UP1InventoryWidget::UpdateGold(const int64 Gold)
{
    Gold_txt->SetText(FText::AsNumber(Gold));
}
