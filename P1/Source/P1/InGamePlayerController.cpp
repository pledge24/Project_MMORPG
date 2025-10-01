// Fill out your copyright notice in the Description page of Project Settings.

#include "InGamePlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "StatusWindowWidget.h"
#include "InventoryWidget.h"
#include "HUDWidget.h"
#include "P1GameInstance.h"
#include "P1Player.h"
#include "P1MyPlayer.h"
#include "P1.h"

void AInGamePlayerController::BeginPlay()
{
    Super::BeginPlay();

    Protocol::C_ENTER_MAP_COMPLETE pkt;
    SEND_PACKET(pkt);

    if(HUDWidgetClass && !HUDWidget)
    {
        HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
        if (HUDWidget)
        {
            HUDWidget->AddToViewport();
        }
    }

    if (HelpWidgetClass && !HelpWidget)
    {
        HelpWidget = CreateWidget<UUserWidget>(this, HelpWidgetClass);
        if (HelpWidget)
        {
            HelpWidget->AddToViewport();
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
        ShopWidget = CreateWidget<UUserWidget>(this, ShopWidgetClass);
        if (ShopWidget)
        {
            ShopWidget->AddToViewport();
            ShopWidget->SetVisibility(ESlateVisibility::Collapsed);
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

void AInGamePlayerController::TurnOnWidget(WidgetType Type)
{
    if (UUserWidget* Widget = WidgetMappings[Type])
    {
        Widget->RemoveFromViewport();
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

