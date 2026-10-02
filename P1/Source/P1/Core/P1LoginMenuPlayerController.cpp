#include "Core/P1LoginMenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "UI/Frontend/P1LoginMenuWidget.h"

AP1LoginMenuPlayerController::AP1LoginMenuPlayerController()
{
    LoginMenuWidget = nullptr;
}

void AP1LoginMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (LoginMenuWidgetClass && !LoginMenuWidget)
    {
        LoginMenuWidget = CreateWidget<UP1LoginMenuWidget>(this, LoginMenuWidgetClass);
        if (LoginMenuWidget)
        {
            LoginMenuWidget->AddToViewport();
        }
    }

}
