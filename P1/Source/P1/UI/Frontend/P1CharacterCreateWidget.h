#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Lobby/P1CharacterClass.h"
#include "P1CharacterCreateWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class UP1LobbySubsystem;

DECLARE_MULTICAST_DELEGATE(FOnCharacterCreateClosed);

/**
 * 캐릭터 생성 화면.
 *
 * 로비가 알려 준 생성할 수 있는 직업 중 하나와 이름을 받아 캐릭터 생성을 요청하고, 거절된 사유를 보여 준다.
 */
UCLASS()
class P1_API UP1CharacterCreateWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;

    /** 로비의 델리게이트에서 이 위젯을 뗀다. */
    virtual void NativeDestruct() override;
    //~ End UUserWidget Interface

    //~ Character Create
public:
    /**
     * 화면을 열기 전에 부른다. 이름과 문구를 비우고 생성할 수 있는 직업의 버튼만 보인다.
     * 생성할 수 있는 직업이 하나뿐이면 그 직업을 골라 둔다.
     */
    void Reset();

    /** 취소 버튼을 누르면 알린다. 생성에 성공한 것은 로비의 목록 변경 알림으로 알 수 있다. */
    FOnCharacterCreateClosed OnClosed;

private:
    /** 게임 인스턴스가 없으면 nullptr. */
    UP1LobbySubsystem* GetLobby() const;

    void SelectClass(EP1CharacterClass CharacterClass);

    /** 서버가 생성을 거절한 결과의 문구를 보여 준다. */
    void ShowCreateFailed(const FText& Message);

    UFUNCTION()
    void OnWarriorButtonClicked();

    UFUNCTION()
    void OnMageButtonClicked();

    UFUNCTION()
    void OnCreateButtonClicked();

    UFUNCTION()
    void OnCancelButtonClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> WarriorButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> MageButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> NameBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SelectedClassText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DescriptionText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CreateButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CancelButton;

    /** 고르지 않았으면 None. */
    EP1CharacterClass SelectedClass = EP1CharacterClass::None;
};
