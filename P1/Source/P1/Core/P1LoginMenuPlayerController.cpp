#include "Core/P1LoginMenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Online/P1LoginManager.h"
#include "UI/Frontend/P1LoginWidget.h"

AP1LoginMenuPlayerController::AP1LoginMenuPlayerController()
{
    LoginMenuWidget = nullptr;
}

void AP1LoginMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();

    LoginManager = NewObject<UP1LoginManager>(this, UP1LoginManager::StaticClass());

    if (LoginMenuWidgetClass && !LoginMenuWidget)
    {
        LoginMenuWidget = CreateWidget<UP1LoginWidget>(this, LoginMenuWidgetClass);
        if (LoginMenuWidget)
        {
            LoginMenuWidget->AddToViewport();
        }
        LoginManager->SetLoginWidget(LoginMenuWidget);
    }

}
