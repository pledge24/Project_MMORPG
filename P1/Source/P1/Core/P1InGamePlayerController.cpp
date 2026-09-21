#include "Core/P1InGamePlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Screens/P1StatusWindowWidget.h"
#include "UI/Screens/P1InventoryWidget.h"
#include "UI/Screens/P1HUDWidget.h"
#include "Core/P1GameInstance.h"
#include "Game/Entities/P1Player.h"
#include "Game/Entities/P1MyPlayer.h"
#include "P1.h"
#include "UI/Screens/P1ShopWidget.h"
#include "UI/WorldSpace/P1NameplateWidget.h"
#include "UI/Screens/P1WarningTextWidget.h"
#include "UI/Screens/P1DeathWidget.h"
#include "Core/P1MyPlayerData.h"

void AP1InGamePlayerController::BeginPlay()
{
    Super::BeginPlay();

    Protocol::C_ENTER_ROOM EnterRoomPkt;
    {
        EnterRoomPkt.set_enter_type(Protocol::ENTER_TYPE_CROSS_MAP_TRANSFER);

        if (UP1MyPlayerData* MyPlayerData = GetGameInstance()->GetSubsystem<UP1MyPlayerData>())
        {
            int32 RoomId = MyPlayerData->GetRoomId();
            EnterRoomPkt.set_room_id(RoomId);
        }

        SEND_PACKET(EnterRoomPkt);
    }

    // ==================== Widget들 추가 ======================
    if(HUDWidgetClass && !HUDWidget)
    {
        HUDWidget = CreateWidget<UP1HUDWidget>(this, HUDWidgetClass);
        if (HUDWidget)
        {
            HUDWidget->AddToViewport();
            HUDWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }

    if (HelpWidgetClass && !HelpWidget)
    {
        HelpWidget = CreateWidget<UUserWidget>(this, HelpWidgetClass);
        if (HelpWidget)
        {
            HelpWidget->AddToViewport();
            HelpWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }

    if (StatusWindowWidgetClass && !StatusWindowWidget)
    {
        StatusWindowWidget = CreateWidget<UP1StatusWindowWidget>(this, StatusWindowWidgetClass);
        if (StatusWindowWidget)
        {
            StatusWindowWidget->AddToViewport();
            StatusWindowWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (InventoryWidgetClass && !InventoryWidget)
    {
        InventoryWidget = CreateWidget<UP1InventoryWidget>(this, InventoryWidgetClass);
        if (InventoryWidget)
        {
            InventoryWidget->AddToViewport();
            InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (ShopWidgetClass && !ShopWidget)
    {
        ShopWidget = CreateWidget<UP1ShopWidget>(this, ShopWidgetClass);
        if (ShopWidget)
        {
            ShopWidget->AddToViewport();
            ShopWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (WarningTextWidgetClass && !WarningTextWidget)
    {
        WarningTextWidget = CreateWidget<UP1WarningTextWidget>(this, WarningTextWidgetClass);
        if (WarningTextWidget)
        {
            WarningTextWidget->AddToViewport();
            WarningTextWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }

    if (DeathWidgetClass && !DeathWidget)
    {
        DeathWidget = CreateWidget<UP1DeathWidget>(this, DeathWidgetClass);
        if (DeathWidget)
        {
            DeathWidget->AddToViewport(DEATH_WIDGET_Z_ORDER);
            DeathWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    WidgetMappings = {
        {EP1WidgetType::WIDGET_STATUS_WINDOW, StatusWindowWidget},
        {EP1WidgetType::WIDGET_INVENTORY, InventoryWidget},
        {EP1WidgetType::WIDGET_SHOP, ShopWidget},
    };
    
}

void AP1InGamePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAction("ToggleStatusWindow", IE_Pressed, this, &AP1InGamePlayerController::OnToggleStatusWindowWidget);
    InputComponent->BindAction("ToggleInventory", IE_Pressed, this, &AP1InGamePlayerController::OnToggleInventoryWidget);
}

void AP1InGamePlayerController::OnToggleStatusWindowWidget()
{
    ToggleWidget(EP1WidgetType::WIDGET_STATUS_WINDOW);
}

void AP1InGamePlayerController::OnToggleInventoryWidget()
{
    ToggleWidget(EP1WidgetType::WIDGET_INVENTORY);
}

void AP1InGamePlayerController::TurnOnWidget(EP1WidgetType Type)
{
    if (UUserWidget* Widget = WidgetMappings[Type])
    {
        Widget->RemoveFromParent();
        Widget->AddToViewport(CurrentMaxZOrder++);
        Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        uint8 FlagIdx = (uint8)Type;

        // Update Widget Flag
        WidgetFlag |= (1 << FlagIdx);

        // 켜진 UI가 1개 이상이면 UI모드 유지
        bShowMouseCursor = true;
        SetInputMode(FInputModeGameAndUI());
    }
}

void AP1InGamePlayerController::TurnOffWidget(EP1WidgetType Type)
{
    if (UUserWidget* Widget = WidgetMappings[Type])
    {
        ESlateVisibility Visibility = Widget->GetVisibility();
        if (Visibility == ESlateVisibility::Collapsed || Visibility == ESlateVisibility::Hidden)
            return;

        Widget->SetVisibility(ESlateVisibility::Collapsed);

        uint8 FlagIdx = (uint8)Type;

        // Update Widget Flag
        WidgetFlag &= ~(1 << FlagIdx);

        // 켜진 UI가 1개 이상이면 UI모드 유지
        if (WidgetFlag > 0)
        {
            bShowMouseCursor = true;
            SetInputMode(FInputModeGameAndUI());
        }
        else
        {
            bShowMouseCursor = false;
            SetInputMode(FInputModeGameOnly());
        }
    }
}

bool AP1InGamePlayerController::IsTurnOnThisWidget(EP1WidgetType Type) const
{
    return WidgetFlag & (1 << (uint8)Type); 
}

void AP1InGamePlayerController::DisplayWarningText(const FText& Message)
{
    WarningTextWidget->DisplayWarningMessage(Message);
}

void AP1InGamePlayerController::ToggleWidget(EP1WidgetType Type)
{
    uint8 FlagIdx = (uint8)Type;
    bool IsActive = (WidgetFlag & (1 << FlagIdx)) > 0;

    if (!IsActive)
    {
        TurnOnWidget(Type);
    }
    else
    {
        TurnOffWidget(Type);
    }
}

void AP1InGamePlayerController::OnToggleBattleMode(bool BattleMode)
{
    HUDWidget->SetBattleModeTxt(BattleMode);

    if (BattleMode)
    {
        FString WarningMessage = TEXT("전투모드를 활성화합니다");
        DisplayWarningText(FText::FromString(WarningMessage));
    }
    else
    {
        FString WarningMessage = TEXT("전투모드를 비활성화합니다");
        DisplayWarningText(FText::FromString(WarningMessage));
    }
}
