#include "UI/P1ScreenSubsystem.h"
#include "UI/P1UISettings.h"
#include "UI/Screens/P1HUDWidget.h"
#include "UI/Screens/P1StatusWindowWidget.h"
#include "UI/Screens/P1InventoryWidget.h"
#include "UI/Screens/P1ShopWidget.h"
#include "UI/Screens/P1WarningTextWidget.h"
#include "UI/Screens/P1DeathWidget.h"
#include "UI/Screens/P1WindowLayerWidget.h"
#include "GameFramework/PlayerController.h"
#include "Utils/LogCategory.h"

namespace
{
    // 뷰포트의 Z 순서다. 값이 같으면 나중에 붙인 것이 위에 그려진다.
    constexpr int32 BACKGROUND_Z_ORDER = 0;     // HUD, 도움말
    constexpr int32 WINDOW_LAYER_Z_ORDER = 1;
    constexpr int32 WARNING_TEXT_Z_ORDER = 2;
    constexpr int32 DEATH_SCREEN_Z_ORDER = 10000;

    template <typename T>
    T* LoadAndCreate(APlayerController* OwningPlayer, const TSoftClassPtr<T>& WidgetClass)
    {
        UClass* LoadedClass = WidgetClass.LoadSynchronous();
        if (LoadedClass == nullptr)
        {
            UE_LOG(LogP1UI, Warning, TEXT("P1 UI 설정의 위젯 클래스가 비어 있거나 불러오지 못했다: %s"), *WidgetClass.ToString());
            return nullptr;
        }

        return CreateWidget<T>(OwningPlayer, LoadedClass);
    }

    template <typename T>
    T* CreateScreen(APlayerController* OwningPlayer, const TSoftClassPtr<T>& WidgetClass, int32 ZOrder, ESlateVisibility Visibility)
    {
        T* Widget = LoadAndCreate(OwningPlayer, WidgetClass);
        if (Widget)
        {
            Widget->AddToViewport(ZOrder);
            Widget->SetVisibility(Visibility);
        }
        return Widget;
    }

    template <typename T>
    T* CreateWindowWidget(APlayerController* OwningPlayer, const TSoftClassPtr<T>& WidgetClass, UP1WindowLayerWidget* WindowLayer)
    {
        T* Window = LoadAndCreate(OwningPlayer, WidgetClass);
        if (Window)
        {
            WindowLayer->AddWindow(Window);
            Window->SetVisibility(ESlateVisibility::Collapsed);
        }
        return Window;
    }
}

void UP1ScreenSubsystem::CreateScreens(APlayerController* InOwningPlayer)
{
    if (InOwningPlayer == nullptr || WindowLayer != nullptr)
        return;

    OwningPlayer = InOwningPlayer;
    const UP1UISettings* Settings = GetDefault<UP1UISettings>();

    // 붙이는 순서가 같은 Z 순서 안에서의 앞뒤를 정한다.
    HUDWidget = CreateScreen(InOwningPlayer, Settings->HUDWidgetClass, BACKGROUND_Z_ORDER, ESlateVisibility::HitTestInvisible);
    HelpWidget = CreateScreen(InOwningPlayer, Settings->HelpWidgetClass, BACKGROUND_Z_ORDER, ESlateVisibility::HitTestInvisible);

    WindowLayer = CreateWidget<UP1WindowLayerWidget>(InOwningPlayer, UP1WindowLayerWidget::StaticClass());
    WindowLayer->AddToViewport(WINDOW_LAYER_Z_ORDER);
    WindowLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    WindowWidgets = {
        {EP1WidgetType::WIDGET_STATUS_WINDOW, CreateWindowWidget(InOwningPlayer, Settings->StatusWindowWidgetClass, WindowLayer)},
        {EP1WidgetType::WIDGET_INVENTORY, CreateWindowWidget(InOwningPlayer, Settings->InventoryWidgetClass, WindowLayer)},
        {EP1WidgetType::WIDGET_SHOP, CreateWindowWidget(InOwningPlayer, Settings->ShopWidgetClass, WindowLayer)},
    };

    WarningTextWidget = CreateScreen(InOwningPlayer, Settings->WarningTextWidgetClass, WARNING_TEXT_Z_ORDER, ESlateVisibility::HitTestInvisible);
    DeathWidget = CreateScreen(InOwningPlayer, Settings->DeathWidgetClass, DEATH_SCREEN_Z_ORDER, ESlateVisibility::Collapsed);
}

void UP1ScreenSubsystem::ReleaseScreens()
{
    for (UUserWidget* Widget : TArray<UUserWidget*>{ HUDWidget, HelpWidget, WindowLayer, WarningTextWidget, DeathWidget })
    {
        if (Widget)
            Widget->RemoveFromParent();
    }

    HUDWidget = nullptr;
    HelpWidget = nullptr;
    WindowLayer = nullptr;
    WarningTextWidget = nullptr;
    DeathWidget = nullptr;
    WindowWidgets.Empty();
    OpenWindowFlags = 0;
    OwningPlayer = nullptr;
}

void UP1ScreenSubsystem::OpenWindow(EP1WidgetType Type)
{
    UUserWidget* Window = FindWindow(Type);
    if (Window == nullptr)
        return;

    WindowLayer->BringToFront(Window);
    Window->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    OpenWindowFlags |= (1 << static_cast<uint8>(Type));

    RefreshInputMode();
}

void UP1ScreenSubsystem::CloseWindow(EP1WidgetType Type)
{
    UUserWidget* Window = FindWindow(Type);
    if (Window == nullptr)
        return;

    ESlateVisibility Visibility = Window->GetVisibility();
    if (Visibility == ESlateVisibility::Collapsed || Visibility == ESlateVisibility::Hidden)
        return;

    Window->SetVisibility(ESlateVisibility::Collapsed);

    OpenWindowFlags &= ~(1 << static_cast<uint8>(Type));

    RefreshInputMode();
}

void UP1ScreenSubsystem::ToggleWindow(EP1WidgetType Type)
{
    if (IsWindowOpen(Type))
        CloseWindow(Type);
    else
        OpenWindow(Type);
}

bool UP1ScreenSubsystem::IsWindowOpen(EP1WidgetType Type) const
{
    return (OpenWindowFlags & (1 << static_cast<uint8>(Type))) != 0;
}

void UP1ScreenSubsystem::RefreshInputMode()
{
    APlayerController* PC = OwningPlayer.Get();
    if (PC == nullptr)
        return;

    if (OpenWindowFlags != 0)
    {
        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeGameAndUI());
    }
    else
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
}

UUserWidget* UP1ScreenSubsystem::FindWindow(EP1WidgetType Type) const
{
    const TObjectPtr<UUserWidget>* Found = WindowWidgets.Find(Type);
    if (Found == nullptr || *Found == nullptr)
    {
        UE_LOG(LogP1UI, Warning, TEXT("등록되지 않은 창 종류다: %d"), static_cast<int32>(Type));
        return nullptr;
    }

    return *Found;
}

void UP1ScreenSubsystem::DisplayWarningText(const FText& Message)
{
    if (WarningTextWidget)
        WarningTextWidget->DisplayWarningMessage(Message);
}

void UP1ScreenSubsystem::ShowBattleMode(bool bBattleMode)
{
    if (HUDWidget)
        HUDWidget->SetBattleModeTxt(bBattleMode);

    if (bBattleMode)
        DisplayWarningText(FText::FromString(TEXT("전투모드를 활성화합니다")));
    else
        DisplayWarningText(FText::FromString(TEXT("전투모드를 비활성화합니다")));
}
