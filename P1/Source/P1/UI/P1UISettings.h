#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "P1UISettings.generated.h"

class UP1UserWidget;
class UP1HUDWidget;
class UP1ChatWidget;
class UP1StatusWindowWidget;
class UP1InventoryWidget;
class UP1ShopWidget;
class UP1WarningTextWidget;
class UP1DeathWidget;
class UP1GameMenuWidget;
class UP1EntryProgressWidget;

/**
 * 인게임 UI 위젯 클래스 설정.
 *
 * 인게임 화면과 창, 레벨을 건너 덮는 로딩 화면에 띄울 위젯 클래스를 프로젝트 설정 「P1 UI」에서 고르게 한다.
 * 기본값은 Config/DefaultGame.ini에 저장된다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "P1 UI"))
class P1_API UP1UISettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** HUD 위젯 클래스. */
    UPROPERTY(Config, EditAnywhere, Category = "Screens")
    TSoftClassPtr<UP1HUDWidget> HUDWidgetClass;

    /** 조작 도움말 위젯 클래스. 로직 없는 표시 위젯이라 공용 베이스를 그대로 쓴다. */
    UPROPERTY(Config, EditAnywhere, Category = "Screens")
    TSoftClassPtr<UP1UserWidget> HelpWidgetClass;

    /** 게임 화면 왼쪽 아래의 채팅창 위젯 클래스. */
    UPROPERTY(Config, EditAnywhere, Category = "Screens")
    TSoftClassPtr<UP1ChatWidget> ChatWidgetClass;

    /** 경고 문구 위젯 클래스. */
    UPROPERTY(Config, EditAnywhere, Category = "Screens")
    TSoftClassPtr<UP1WarningTextWidget> WarningTextWidgetClass;

    /** 사망 화면 위젯 클래스. */
    UPROPERTY(Config, EditAnywhere, Category = "Screens")
    TSoftClassPtr<UP1DeathWidget> DeathWidgetClass;

    /** 메뉴 키로 여는 게임 메뉴 위젯 클래스. */
    UPROPERTY(Config, EditAnywhere, Category = "Screens")
    TSoftClassPtr<UP1GameMenuWidget> GameMenuWidgetClass;

    /** 스탯 창 위젯 클래스. */
    UPROPERTY(Config, EditAnywhere, Category = "Windows")
    TSoftClassPtr<UP1StatusWindowWidget> StatusWindowWidgetClass;

    /** 인벤토리 창 위젯 클래스. */
    UPROPERTY(Config, EditAnywhere, Category = "Windows")
    TSoftClassPtr<UP1InventoryWidget> InventoryWidgetClass;

    /** 상점 창 위젯 클래스. */
    UPROPERTY(Config, EditAnywhere, Category = "Windows")
    TSoftClassPtr<UP1ShopWidget> ShopWidgetClass;

    /** 월드에 접속하거나 맵을 불러오는 동안 화면을 덮는 접속 진행 표시 위젯 클래스. */
    UPROPERTY(Config, EditAnywhere, Category = "Loading")
    TSoftClassPtr<UP1EntryProgressWidget> LoadingScreenWidgetClass;
};
