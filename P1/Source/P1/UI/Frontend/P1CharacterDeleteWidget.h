#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1CharacterDeleteWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
struct FP1LobbyCharacter;

DECLARE_MULTICAST_DELEGATE(FOnCharacterDeleteClosed);

/**
 * 캐릭터 삭제 확인 창.
 *
 * 삭제할 캐릭터를 보여 주고, 그 캐릭터의 이름을 그대로 입력해야 삭제를 요청할 수 있게 한다.
 */
UCLASS()
class P1_API UP1CharacterDeleteWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    //~ End UUserWidget Interface

    //~ Character Delete
public:
    /** 창을 열기 전에 부른다. 이름 입력을 비우고 삭제 버튼을 끈다. */
    void SetCharacter(const FP1LobbyCharacter& Character);

    /** 취소 버튼을 누르면 알린다. 삭제의 결과는 로비의 알림으로 알 수 있다. */
    FOnCharacterDeleteClosed OnClosed;

private:
    UFUNCTION()
    void OnNameChanged(const FText& Text);

    UFUNCTION()
    void OnConfirmButtonClicked();

    UFUNCTION()
    void OnCancelButtonClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> CharacterInfoText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> NameBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ConfirmButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CancelButton;

    /** 창을 연 캐릭터가 없으면 -1. */
    int64 CharacterId = -1;

    /** 이름 입력이 이 값과 같아야 삭제 버튼이 켜진다. */
    FString CharacterName;
};
