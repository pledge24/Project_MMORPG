// Fill out your copyright notice in the Description page of Project Settings.

#include "InGamePlayerController.h"
#include "Blueprint/UserWidget.h"
#include "StatusWindowWidget.h"
#include "InventoryWidget.h"
#include "HUDWidget.h"

enum WidgetType
{
    WIDGET_NONE = 0,
    WIDGET_STATUS_WINDOW = 1,
    WIDGET_INVENTORY = 2
};

AInGamePlayerController::AInGamePlayerController()
{
    HUDWidget = nullptr;
}

void AInGamePlayerController::BeginPlay()
{
    Super::BeginPlay();

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
}

void AInGamePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction("ToggleStatusWindow", IE_Pressed, this, &AInGamePlayerController::OnToggleStatusWindowWidget);
    InputComponent->BindAction("ToggleInventory", IE_Pressed, this, &AInGamePlayerController::OnToggleInventoryWidget);
}

void AInGamePlayerController::UpdateInventorySlot(const Protocol::Slot& _Slot)
{
    InventoryWidget->UpdateSlot(_Slot);
}

void AInGamePlayerController::UpdateEquippedGearSlot(const Protocol::Slot& _Slot)
{
    StatusWindowWidget->UpdateSlot(_Slot);
}

void AInGamePlayerController::UpdatePlayerUI(const Protocol::PlayerInfo& _PlayerInfo)
{
    StatusWindowWidget->UpdateAllStat(_PlayerInfo.stat_info());
    HUDWidget->UpdateAllProgressBar(_PlayerInfo.stat_info(), _PlayerInfo.cur_exp(), _PlayerInfo.max_exp());
}


void AInGamePlayerController::OnToggleStatusWindowWidget()
{
    ToggleWidget(StatusWindowWidget, WIDGET_STATUS_WINDOW);
}

void AInGamePlayerController::OnToggleInventoryWidget()
{
    ToggleWidget(InventoryWidget, WIDGET_INVENTORY);
}

void AInGamePlayerController::ToggleWidget(UUserWidget* Widget, int32 FlagIdx)
{
    bool IsVisible = (ToggleFlag & (1 << FlagIdx)) > 0;

    if (IsVisible)
    {
        Widget->SetVisibility(ESlateVisibility::Collapsed);
    }
    else
    {
        Widget->SetVisibility(ESlateVisibility::Visible);
    }

    // Toggle Flag
    ToggleFlag ^= (1 << FlagIdx);

    // 켜진 UI가 1개 이상이면 UI모드 유지
    if (ToggleFlag > 0)
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

