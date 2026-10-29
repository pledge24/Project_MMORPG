#include "UI/P1ScreenSubsystem.h"
#include "UI/P1UISettings.h"
#include "Core/P1EntryFlowSubsystem.h"
#include "UI/Screens/P1HUDWidget.h"
#include "UI/Screens/P1ChatWidget.h"
#include "UI/Screens/P1StatusWindowWidget.h"
#include "UI/Screens/P1InventoryWidget.h"
#include "UI/Screens/P1ShopWidget.h"
#include "UI/Screens/P1ChannelListWidget.h"
#include "UI/Screens/P1WarningTextWidget.h"
#include "UI/Screens/P1DeathWidget.h"
#include "UI/Screens/P1GameMenuWidget.h"
#include "UI/Screens/P1WindowLayerWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Game/Progress/P1ItemActions.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Utils/LogCategory.h"

namespace
{
    // 뷰포트의 Z 순서다. 값이 같으면 나중에 붙인 것이 위에 그려진다.
    constexpr int32 BACKGROUND_Z_ORDER = 0;     // HUD, 도움말, 채팅창
    constexpr int32 WINDOW_LAYER_Z_ORDER = 1;
    constexpr int32 WARNING_TEXT_Z_ORDER = 2;
    constexpr int32 DEATH_SCREEN_Z_ORDER = 10000;
    constexpr int32 GAME_MENU_Z_ORDER = 20000;     // 사망한 채로도 메뉴를 열어 로비로 돌아갈 수 있다

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

    // 배경 화면. 붙이는 순서가 같은 Z 순서 안에서의 앞뒤를 정한다.
    // HUD는 퀵슬롯이 끌어 놓기를 받아야 하므로 자식의 입력을 막지 않는다. 입력을 받지 않을 그림과 글자는 WBP가 막는다.
    HUDWidget = CreateScreen(InOwningPlayer, Settings->HUDWidgetClass, BACKGROUND_Z_ORDER, ESlateVisibility::SelfHitTestInvisible);
    HelpWidget = CreateScreen(InOwningPlayer, Settings->HelpWidgetClass, BACKGROUND_Z_ORDER, ESlateVisibility::HitTestInvisible);
    ChatWidget = CreateScreen(InOwningPlayer, Settings->ChatWidgetClass, BACKGROUND_Z_ORDER, ESlateVisibility::SelfHitTestInvisible);

    // 창 레이어와 창. 창은 닫힌 채로 만든다.
    WindowLayer = CreateWidget<UP1WindowLayerWidget>(InOwningPlayer, UP1WindowLayerWidget::StaticClass());
    WindowLayer->AddToViewport(WINDOW_LAYER_Z_ORDER);
    WindowLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    WindowWidgets = {
        {EP1WidgetType::WIDGET_STATUS_WINDOW, CreateWindowWidget(InOwningPlayer, Settings->StatusWindowWidgetClass, WindowLayer)},
        {EP1WidgetType::WIDGET_INVENTORY, CreateWindowWidget(InOwningPlayer, Settings->InventoryWidgetClass, WindowLayer)},
        {EP1WidgetType::WIDGET_SHOP, CreateWindowWidget(InOwningPlayer, Settings->ShopWidgetClass, WindowLayer)},
        {EP1WidgetType::WIDGET_CHANNEL_LIST, CreateWindowWidget(InOwningPlayer, Settings->ChannelListWidgetClass, WindowLayer)},
    };

    // 창보다 위에 그리는 화면.
    WarningTextWidget = CreateScreen(InOwningPlayer, Settings->WarningTextWidgetClass, WARNING_TEXT_Z_ORDER, ESlateVisibility::HitTestInvisible);
    DeathWidget = CreateScreen(InOwningPlayer, Settings->DeathWidgetClass, DEATH_SCREEN_Z_ORDER, ESlateVisibility::Collapsed);
    GameMenuWidget = CreateScreen(InOwningPlayer, Settings->GameMenuWidgetClass, GAME_MENU_Z_ORDER, ESlateVisibility::Collapsed);
}

void UP1ScreenSubsystem::ReleaseScreens()
{
    for (UUserWidget* Widget : TArray<UUserWidget*>{ HUDWidget, HelpWidget, ChatWidget, WindowLayer, WarningTextWidget, DeathWidget, GameMenuWidget })
    {
        if (Widget)
            Widget->RemoveFromParent();
    }

    HUDWidget = nullptr;
    HelpWidget = nullptr;
    ChatWidget = nullptr;
    bChatInputOpen = false;
    WindowLayer = nullptr;
    WarningTextWidget = nullptr;
    DeathWidget = nullptr;
    GameMenuWidget = nullptr;
    WindowWidgets.Empty();
    OpenWindowFlags = 0;
    ShopNpcEntityId = 0;
    OwningPlayer = nullptr;
    SyncShopNpc();
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
    SyncShopNpc();
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
    SyncShopNpc();
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

    // 상점이 열린 동안은 캐릭터 조작과 카메라 회전을 막는다. 카메라는 NPC 상호작용이 NPC 쪽으로 돌린다.
    // 입력은 아래에서 상점 창만 받게 하지만, 상점 위에 채팅 입력칸이나 메뉴가 떴다 닫히는 사이에도 막히도록 따로 건다.
    // 막는 횟수를 쌓지 않도록 매번 풀고 다시 건다.
    PC->ResetIgnoreMoveInput();
    PC->ResetIgnoreLookInput();
    if (IsWindowOpen(EP1WidgetType::WIDGET_SHOP))
    {
        PC->SetIgnoreMoveInput(true);
        PC->SetIgnoreLookInput(true);
    }

    // 게임 메뉴와 사망 화면은 그 화면만 입력을 받는다. 사망 화면이 게임 입력을 막지 않으면 죽은 플레이어가 움직인다.
    // 채팅 입력칸은 사망 화면보다 앞이다. 사망한 채로도 말할 수 있고, 입력칸을 닫으면 다시 사망 화면이 입력을 받는다.
    const bool bChatInputFirst = bChatInputOpen && ChatWidget && IsGameMenuOpen() == false;

    UUserWidget* ModalScreen = nullptr;
    if (IsGameMenuOpen())
        ModalScreen = GameMenuWidget;
    else if (bChatInputFirst == false && DeathWidget && DeathWidget->IsVisible())
        ModalScreen = DeathWidget;

    if (ModalScreen)
    {
        // UI 전용 모드에서 뗀 키는 플레이어 입력에 닿지 않는다. 눌린 키를 먼저 놓지 않으면
        // 화면이 떠 있는 동안 뗀 이동 키가 화면을 닫은 뒤에도 눌린 채 남아 내 플레이어가 계속 움직인다.
        PC->FlushPressedKeys();

        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(ModalScreen->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        PC->bShowMouseCursor = true;
        PC->SetInputMode(InputMode);
    }
    else if (bChatInputFirst)
    {
        // 창이 열린 UI·게임 모드에서는 입력칸이 처리하지 않은 글자 키가 게임으로 넘어가 내 플레이어가 움직인다.
        // 그래서 입력칸만 키를 받게 하고, 눌린 이동 키는 사망 화면과 같은 이유로 먼저 놓는다.
        // 지난 채팅을 휠과 스크롤바로 읽을 수 있게 커서를 보인다.
        PC->FlushPressedKeys();

        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(ChatWidget->GetInputFocusTarget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        PC->bShowMouseCursor = true;
        PC->SetInputMode(InputMode);
    }
    else if (UUserWidget* ShopWindow = IsWindowOpen(EP1WidgetType::WIDGET_SHOP) ? FindWindow(EP1WidgetType::WIDGET_SHOP) : nullptr)
    {
        // 상점은 UI 모드다. 게임 입력이 들어가면 상점을 연 채 전투 모드를 켜고 공격하거나, 퀵슬롯을 쓰거나, 다른 창을 다시 연다.
        // 메뉴 키와 채팅 키는 상점 창이 받는다.
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(ShopWindow->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        PC->bShowMouseCursor = true;
        PC->SetInputMode(InputMode);
    }
    else if (OpenWindowFlags != 0)
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

void UP1ScreenSubsystem::SyncShopNpc() const
{
    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    const UGameInstance* GameInstance = LocalPlayer ? LocalPlayer->GetGameInstance() : nullptr;
    const UP1MyPlayerData* MyPlayerData = GameInstance ? GameInstance->GetSubsystem<UP1MyPlayerData>() : nullptr;
    if (MyPlayerData)
        MyPlayerData->GetItemActions()->SetShopNpc(IsWindowOpen(EP1WidgetType::WIDGET_SHOP) ? ShopNpcEntityId : 0);
}

bool UP1ScreenSubsystem::OpenShop(int64 NpcEntityId, int32 ShopId)
{
    UP1ShopWidget* ShopWidget = Cast<UP1ShopWidget>(FindWindow(EP1WidgetType::WIDGET_SHOP));
    APlayerController* PC = OwningPlayer.Get();
    if (ShopWidget == nullptr || PC == nullptr)
        return false;

    // 상점과 인벤토리만 남긴다. 다른 창은 닫은 채로 두고 상점을 닫아도 되살리지 않는다.
    for (const TPair<EP1WidgetType, TObjectPtr<UUserWidget>>& Window : WindowWidgets)
    {
        if (Window.Key != EP1WidgetType::WIDGET_SHOP && Window.Key != EP1WidgetType::WIDGET_INVENTORY)
            CloseWindow(Window.Key);
    }

    // 조작을 막기 전에 눌린 키를 놓는다. 놓지 않으면 상점을 닫은 뒤 그 키가 눌린 채 남는다.
    PC->FlushPressedKeys();

    ShopNpcEntityId = NpcEntityId;
    ShopWidget->SetShop(ShopId);
    OpenWindow(EP1WidgetType::WIDGET_INVENTORY);
    OpenWindow(EP1WidgetType::WIDGET_SHOP);
    return true;
}

void UP1ScreenSubsystem::CloseShop()
{
    if (IsWindowOpen(EP1WidgetType::WIDGET_SHOP) == false)
        return;

    CloseWindow(EP1WidgetType::WIDGET_SHOP);
    CloseWindow(EP1WidgetType::WIDGET_INVENTORY);
    ShopNpcEntityId = 0;
    SyncShopNpc();
}

void UP1ScreenSubsystem::ToggleGameMenu()
{
    if (GameMenuWidget == nullptr)
        return;

    if (IsGameMenuOpen())
    {
        CloseGameMenu();
        return;
    }

    // 맵을 불러오거나 로비 복귀를 기다리는 동안에는 메뉴의 버튼이 아무것도 하지 않으므로 열지 않는다.
    const UGameInstance* GameInstance = GetLocalPlayer()->GetGameInstance();
    const UP1EntryFlowSubsystem* EntryFlow = GameInstance ? GameInstance->GetSubsystem<UP1EntryFlowSubsystem>() : nullptr;
    if (EntryFlow == nullptr || EntryFlow->GetPhase() != EP1EntryPhase::Game)
        return;

    GameMenuWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    RefreshInputMode();
}

void UP1ScreenSubsystem::CloseGameMenu()
{
    if (IsGameMenuOpen() == false)
        return;

    GameMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
    RefreshInputMode();
}

bool UP1ScreenSubsystem::IsGameMenuOpen() const
{
    return GameMenuWidget && GameMenuWidget->IsVisible();
}

void UP1ScreenSubsystem::OpenChatInput()
{
    if (ChatWidget == nullptr || bChatInputOpen)
        return;

    // 게임 메뉴와 같은 이유로 게임 단계에서만 연다. 서버도 게임 단계의 세션이 보낸 채팅만 받는다.
    const UGameInstance* GameInstance = GetLocalPlayer()->GetGameInstance();
    const UP1EntryFlowSubsystem* EntryFlow = GameInstance ? GameInstance->GetSubsystem<UP1EntryFlowSubsystem>() : nullptr;
    if (EntryFlow == nullptr || EntryFlow->GetPhase() != EP1EntryPhase::Game)
        return;

    bChatInputOpen = true;
    ChatWidget->ShowInput();
    RefreshInputMode();
}

void UP1ScreenSubsystem::CloseChatInput()
{
    if (bChatInputOpen == false)
        return;

    bChatInputOpen = false;
    if (ChatWidget)
        ChatWidget->HideInput();

    RefreshInputMode();
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

void UP1ScreenSubsystem::ShowInteractionPrompt(const FText& Prompt)
{
    if (HUDWidget)
        HUDWidget->SetInteractionPrompt(Prompt);
}
