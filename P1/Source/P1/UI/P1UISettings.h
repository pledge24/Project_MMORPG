#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "P1UISettings.generated.h"

class UUserWidget;
class UP1HUDWidget;
class UP1StatusWindowWidget;
class UP1InventoryWidget;
class UP1ShopWidget;
class UP1WarningTextWidget;
class UP1DeathWidget;

/**
 * 인게임 화면에 띄우는 위젯의 클래스다. 기본값은 Config/DefaultGame.ini에 있고,
 * 에디터의 프로젝트 설정 「P1 UI」에서 바꿀 수 있다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "P1 UI"))
class P1_API UP1UISettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category = "Screens")
    TSoftClassPtr<UP1HUDWidget> HUDWidgetClass;

    /** 조작 도움말이다. 블루프린트에만 있는 표시 위젯이라 C++ 타입이 없다. */
    UPROPERTY(Config, EditAnywhere, Category = "Screens")
    TSoftClassPtr<UUserWidget> HelpWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Screens")
    TSoftClassPtr<UP1WarningTextWidget> WarningTextWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Screens")
    TSoftClassPtr<UP1DeathWidget> DeathWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Windows")
    TSoftClassPtr<UP1StatusWindowWidget> StatusWindowWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Windows")
    TSoftClassPtr<UP1InventoryWidget> InventoryWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Windows")
    TSoftClassPtr<UP1ShopWidget> ShopWidgetClass;
};
