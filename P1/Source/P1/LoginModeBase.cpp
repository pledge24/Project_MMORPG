// Fill out your copyright notice in the Description page of Project Settings.

#include "LoginModeBase.h"
#include "Blueprint/UserWidget.h"
#include "LoginManager.h"
#include "LoginWidget.h"

void ALoginModeBase::StartPlay()
{
    Super::StartPlay();

    // LoginManager 积己
    LoginManager = NewObject<ULoginManager>(this, ULoginManager::StaticClass());

    // 困连 积己
    if (ULoginWidget* LoginWidget = CreateWidget<ULoginWidget>(GetWorld(), LoginWidgetClass))
    {
        LoginWidget->AddToViewport();
        LoginManager->SetLoginWidget(LoginWidget);
    }
}
