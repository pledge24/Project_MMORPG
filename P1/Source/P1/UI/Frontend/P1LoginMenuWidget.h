#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1LoginMenuWidget.generated.h"

class UWidgetSwitcher;
class UP1CharacterCreateWidget;
class UP1CharacterDeleteWidget;
class UP1CharacterSelectWidget;
class UP1EntryProgressWidget;
class UP1LoginWidget;
struct FP1LobbyCharacter;
enum class EP1EntryProgress : uint8;

/**
 * 로그인 맵의 메뉴 위젯.
 *
 * 로그인, 캐릭터 선택, 캐릭터 생성, 캐릭터 삭제 확인 화면 가운데 하나를 보이고, 접속 진행 표시를 그 위에 덮는다.
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
    void ShowCharacterSelect();
    void ShowCharacterCreate();
    void ShowCharacterDelete(const FP1LobbyCharacter& Character);

    /** 이 화면에 있는 동안 게임 서버에 접속하지 못했거나 연결이 끊긴 사유를 로그인 화면에 보여 준다. */
    void ShowLoginNotice(const FString& Notice);

    /** 삭제가 거절되면 확인 창을 닫는다. 사유는 캐릭터 선택 화면이 보여 준다. */
    void HandleDeleteCharacterFailed(const FText& Message);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> ScreenSwitcher;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UP1LoginWidget> LoginScreen;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UP1CharacterSelectWidget> CharacterSelectScreen;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UP1CharacterCreateWidget> CharacterCreateScreen;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UP1CharacterDeleteWidget> CharacterDeleteScreen;

    //~ Entry Progress
private:
    void ShowProgress(EP1EntryProgress Progress);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UP1EntryProgressWidget> ProgressScreen;
};
