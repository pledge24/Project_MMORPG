#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1LoginMenuWidget.generated.h"

class UWidgetSwitcher;
class UP1CharacterCreateWidget;
class UP1CharacterDeleteWidget;
class UP1CharacterSelectWidget;
class UP1LoginWidget;
class UP1WorldSelectWidget;
struct FP1LobbyCharacter;
enum class EP1EntryPhase : uint8;

/**
 * 로그인 맵의 메뉴 위젯.
 *
 * 로그인, 월드 선택, 캐릭터 선택, 캐릭터 생성 화면 가운데 하나를 보이고, 삭제 확인 창을 그 위에 덮는다.
 * 화면 사이를 오가는 순서만 맡고, 각 화면의 일은 그 화면의 위젯이 한다.
 */
UCLASS()
class P1_API UP1LoginMenuWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;

    /** 로비와 접속 흐름의 델리게이트에서 이 위젯을 뗀다. */
    virtual void NativeDestruct() override;
    //~ End UUserWidget Interface

    //~ Screens
private:
    /** 접속 단계가 로그인이나 월드 선택이면 그 화면을 보인다. 로비의 화면은 캐릭터 목록 알림이 연다. */
    void ShowScreenForPhase(EP1EntryPhase Phase);

    void ShowCharacterSelect();
    void ShowCharacterCreate();
    void ShowCharacterDelete(const FP1LobbyCharacter& Character);

    /** 전환기의 화면을 Screen으로 바꾸고 삭제 확인 창을 닫는다. 월드 선택 화면을 떠나면 그 화면의 목록 갱신을 멈춘다. */
    void SetActiveScreen(UWidget* Screen);

    /** 이 맵에 있는 동안 생긴 사유를 지금 단계의 화면(로그인 또는 월드 선택)에 보여 준다. */
    void ShowNotice(const FString& Notice);

    /** 삭제가 거절되면 확인 창을 닫는다. 사유는 캐릭터 선택 화면이 보여 준다. */
    void HandleDeleteCharacterFailed(const FText& Message);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> ScreenSwitcher;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UP1LoginWidget> LoginScreen;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UP1WorldSelectWidget> WorldSelectScreen;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UP1CharacterSelectWidget> CharacterSelectScreen;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UP1CharacterCreateWidget> CharacterCreateScreen;

    /** 화면 전환기 밖에서 캐릭터 선택 화면 위에 덮는 확인 창. 보이고 숨기는 것으로 여닫는다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UP1CharacterDeleteWidget> CharacterDeleteScreen;
};
