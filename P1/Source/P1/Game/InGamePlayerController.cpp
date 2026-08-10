// Fill out your copyright notice in the Description page of Project Settings.

#include "InGamePlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "StatusWindowWidget.h"
#include "InventoryWidget.h"
#include "HUDWidget.h"
#include "P1GameInstance.h"
#include "Objects/P1Player.h"
#include "Objects/P1MyPlayer.h"
#include "P1.h"
#include "ShopWidget.h"
#include "NameplateWidget.h"
#include "WarningTextWidget.h"
#include "DeathWidget.h"
#include "MyPlayerData.h"

void AInGamePlayerController::BeginPlay()
{
    Super::BeginPlay();

    Protocol::C_ENTER_ROOM EnterRoomPkt;
    {
        EnterRoomPkt.set_enter_type(Protocol::ENTER_TYPE_CROSS_MAP_TRANSFER);

        if (UMyPlayerData* MyPlayerData = GetGameInstance()->GetSubsystem<UMyPlayerData>())
        {
            int32 RoomId = MyPlayerData->GetRoomId();
            EnterRoomPkt.set_room_id(RoomId);
        }

        SEND_PACKET(EnterRoomPkt);
    }

    // ==================== Widget들 추가 ======================
    if(HUDWidgetClass && !HUDWidget)
    {
        HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
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
        StatusWindowWidget = CreateWidget<UStatusWindowWidget>(this, StatusWindowWidgetClass);
        if (StatusWindowWidget)
        {
            StatusWindowWidget->AddToViewport();
            StatusWindowWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (InventoryWidgetClass && !InventoryWidget)
    {
        InventoryWidget = CreateWidget<UInventoryWidget>(this, InventoryWidgetClass);
        if (InventoryWidget)
        {
            InventoryWidget->AddToViewport();
            InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (ShopWidgetClass && !ShopWidget)
    {
        ShopWidget = CreateWidget<UShopWidget>(this, ShopWidgetClass);
        if (ShopWidget)
        {
            ShopWidget->AddToViewport();
            ShopWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (WarningTextWidgetClass && !WarningTextWidget)
    {
        WarningTextWidget = CreateWidget<UWarningTextWidget>(this, WarningTextWidgetClass);
        if (WarningTextWidget)
        {
            WarningTextWidget->AddToViewport();
            WarningTextWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }

    if (DeathWidgetClass && !DeathWidget)
    {
        DeathWidget = CreateWidget<UDeathWidget>(this, DeathWidgetClass);
        if (DeathWidget)
        {
            DeathWidget->AddToViewport(DEATH_WIDGET_Z_ORDER);
            DeathWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    WidgetMappings = {
        {WidgetType::WIDGET_STATUS_WINDOW, StatusWindowWidget},
        {WidgetType::WIDGET_INVENTORY, InventoryWidget},
        {WidgetType::WIDGET_SHOP, ShopWidget},
    };
    
}

void AInGamePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAction("ToggleStatusWindow", IE_Pressed, this, &AInGamePlayerController::OnToggleStatusWindowWidget);
    InputComponent->BindAction("ToggleInventory", IE_Pressed, this, &AInGamePlayerController::OnToggleInventoryWidget);
}

void AInGamePlayerController::OnToggleStatusWindowWidget()
{
    ToggleWidget(WidgetType::WIDGET_STATUS_WINDOW);
}

void AInGamePlayerController::OnToggleInventoryWidget()
{
    ToggleWidget(WidgetType::WIDGET_INVENTORY);
}

void AInGamePlayerController::TurnOnWidget(WidgetType Type)
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

void AInGamePlayerController::TurnOffWidget(WidgetType Type)
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

bool AInGamePlayerController::IsTurnOnThisWidget(WidgetType Type) const
{
    return WidgetFlag & (1 << (uint8)Type); 
}

void AInGamePlayerController::DisplayWarningText(const FText& Message)
{
    WarningTextWidget->DisplayWarningMessage(Message);
}

void AInGamePlayerController::ToggleWidget(WidgetType Type)
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

void AInGamePlayerController::OnToggleBattleMode(bool BattleMode)
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
