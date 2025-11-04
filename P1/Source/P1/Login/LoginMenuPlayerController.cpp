// Fill out your copyright notice in the Description page of Project Settings.

#include "LoginMenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "LoginManager.h"
#include "LoginWidget.h"

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
