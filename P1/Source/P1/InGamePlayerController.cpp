// Fill out your copyright notice in the Description page of Project Settings.

#include "InGamePlayerController.h"
#include "Blueprint/UserWidget.h"

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
        HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
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
}

void AInGamePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction("ToggleStatusWindow", IE_Pressed, this, &AInGamePlayerController::ToggleStatusWindow);
    InputComponent->BindAction("ToggleInventory", IE_Pressed, this, &AInGamePlayerController::ToggleInventory);
}

void AInGamePlayerController::ToggleStatusWindow()
{
    // 위젯이 없으면 생성.
    if (!StatusWindowWidget && StatusWindowWidgetClass)
    {
        StatusWindowWidget = CreateWidget<UUserWidget>(this, StatusWindowWidgetClass);
    }

    if (StatusWindowWidget)
        ToggleWidget(StatusWindowWidget, WidgetType::WIDGET_STATUS_WINDOW);
}

void AInGamePlayerController::ToggleInventory()
{
    // 위젯이 없으면 생성.
    if (!InventoryWidget && InventoryWidgetClass)
    {
        InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass);
    }

    if (InventoryWidget)
        ToggleWidget(InventoryWidget, WidgetType::WIDGET_INVENTORY);
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


