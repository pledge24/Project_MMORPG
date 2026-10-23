#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/P1WidgetType.h"
#include "P1ScreenSubsystem.generated.h"

class UUserWidget;
class UP1UserWidget;
class UP1HUDWidget;
class UP1WarningTextWidget;
class UP1DeathWidget;
class UP1GameMenuWidget;
class UP1WindowLayerWidget;

/**
 * 인게임 화면과 창의 관리자.
 *
 * HUD, 도움말, 경고 문구, 사망 화면, 게임 메뉴 같은 인게임 화면과 스탯 창, 인벤토리, 상점 같은 창을 띄우고 여닫는다.
 * 열린 창에 맞춰 입력 모드와 마우스 커서를 바꾼다.
 */
UCLASS()
class P1_API UP1ScreenSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

    //~ Screen Lifecycle
public:
    /** 인게임 컨트롤러의 BeginPlay에서 부른다. 이미 만들었으면 아무것도 하지 않는다. */
    void CreateScreens(APlayerController* InOwningPlayer);

    /** 인게임 컨트롤러의 EndPlay에서 부른다. */
    void ReleaseScreens();

private:
    TWeakObjectPtr<APlayerController> OwningPlayer;

    UPROPERTY()
    TObjectPtr<UP1HUDWidget> HUDWidget;

    UPROPERTY()
    TObjectPtr<UP1UserWidget> HelpWidget;

    UPROPERTY()
    TObjectPtr<UP1WarningTextWidget> WarningTextWidget;

    UPROPERTY()
    TObjectPtr<UP1DeathWidget> DeathWidget;

    //~ Windows
public:
    /** 창을 열어 다른 창보다 앞에 그리고 UI 입력 모드로 바꾼다. 이미 열려 있어도 앞으로 올린다. */
    void OpenWindow(EP1WidgetType Type);

    /** 창을 닫는다. 열린 창이 남지 않으면 게임 입력 모드로 돌아간다. */
    void CloseWindow(EP1WidgetType Type);
    void ToggleWindow(EP1WidgetType Type);
    bool IsWindowOpen(EP1WidgetType Type) const;

    /**
     * 게임 메뉴나 사망 화면이 떠 있으면 그 화면만 입력을 받게 하고, 아니면 열린 창이 하나라도 있으면 UI 모드,
     * 없으면 게임 모드로 입력과 커서를 맞춘다.
     */
    void RefreshInputMode();

private:
    /** 등록되지 않은 종류면 경고를 남기고 nullptr. */
    UUserWidget* FindWindow(EP1WidgetType Type) const;

    /** 상점 창이 열려 있는지를 아이템 요청 창구에 맞춘다. 판매는 상점 창이 열려 있을 때만 보낸다. */
    void SyncShopOpen() const;

    UPROPERTY()
    TObjectPtr<UP1WindowLayerWidget> WindowLayer;

    UPROPERTY()
    TMap<EP1WidgetType, TObjectPtr<UUserWidget>> WindowWidgets;

    /** 열린 창의 비트 집합. EP1WidgetType 값이 비트 위치다. */
    int32 OpenWindowFlags = 0;

    //~ Game Menu
public:
    /** 게임 메뉴를 열거나 닫는다. 게임 단계에서만 열고, 열면 모든 화면 위에 그리며 입력을 게임 메뉴만 받는다. */
    void ToggleGameMenu();
    void CloseGameMenu();
    bool IsGameMenuOpen() const;

private:
    UPROPERTY()
    TObjectPtr<UP1GameMenuWidget> GameMenuWidget;

    //~ Notices
public:
    void DisplayWarningText(const FText& Message);

    /** 내 플레이어의 전투 모드가 바뀌면 부른다. HUD 문구와 경고 문구로 알린다. */
    void ShowBattleMode(bool bBattleMode);
};
