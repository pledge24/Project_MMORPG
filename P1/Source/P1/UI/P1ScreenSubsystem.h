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
class UP1WindowLayerWidget;

/**
 * 인게임 화면(HUD, 도움말, 경고 문구, 사망 화면)과 여닫는 창(스탯 창, 인벤토리, 상점)을 맡는다.
 * 위젯은 인게임 맵마다 한 번 만들고, 창은 표시 여부와 앞뒤 순서만 바꾼다.
 * 위젯 클래스는 UP1UISettings에서 읽는다.
 */
UCLASS()
class P1_API UP1ScreenSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

    //~ Screen Lifecycle
public:
    /** 인게임 컨트롤러의 BeginPlay에서 부른다. 이미 만들었으면 아무것도 하지 않는다. */
    void CreateScreens(APlayerController* InOwningPlayer);

    /** 인게임 컨트롤러의 EndPlay에서 부른다. 위젯을 뷰포트에서 떼고 놓는다. */
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
    /** 창을 보이고 다른 창보다 앞에 그린다. 이미 열려 있어도 앞으로 올린다. */
    void OpenWindow(EP1WidgetType Type);

    void CloseWindow(EP1WidgetType Type);
    void ToggleWindow(EP1WidgetType Type);
    bool IsWindowOpen(EP1WidgetType Type) const;

    /** 열린 창이 하나라도 있으면 UI 모드, 없으면 게임 모드로 입력과 커서를 맞춘다. */
    void RefreshInputMode();

private:
    /** 등록되지 않은 종류면 경고를 남기고 nullptr을 돌려준다. */
    UUserWidget* FindWindow(EP1WidgetType Type) const;

    UPROPERTY()
    TObjectPtr<UP1WindowLayerWidget> WindowLayer;

    UPROPERTY()
    TMap<EP1WidgetType, TObjectPtr<UUserWidget>> WindowWidgets;

    /** 창 종류 값의 비트가 켜져 있으면 그 창이 열려 있다. */
    int32 OpenWindowFlags = 0;

    //~ Notices
public:
    void DisplayWarningText(const FText& Message);

    /** 내 플레이어의 전투 모드 알림을 받는다. HUD 문구를 바꾸고 경고 문구를 띄운다. */
    void ShowBattleMode(bool bBattleMode);
};
