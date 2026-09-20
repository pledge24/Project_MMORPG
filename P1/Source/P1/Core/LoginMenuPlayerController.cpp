// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/LoginMenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Online/LoginManager.h"
#include "UI/LoginWidget.h"

ALoginMenuPlayerController::ALoginMenuPlayerController()
{
    LoginMenuWidget = nullptr;
}

void ALoginMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();

    LoginManager = NewObject<ULoginManager>(this, ULoginManager::StaticClass());

    if (LoginMenuWidgetClass && !LoginMenuWidget)
    {
        LoginMenuWidget = CreateWidget<ULoginWidget>(this, LoginMenuWidgetClass);
        if (LoginMenuWidget)
        {
            LoginMenuWidget->AddToViewport();
        }
        LoginManager->SetLoginWidget(LoginMenuWidget);
    }

}
