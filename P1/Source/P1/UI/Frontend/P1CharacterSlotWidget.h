#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Lobby/P1CharacterClass.h"
#include "P1CharacterSlotWidget.generated.h"

class UButton;
class UEditableTextBox;
class UImage;
class UTextBlock;
struct FP1LobbyCharacter;

DECLARE_DELEGATE_OneParam(FOnCharacterSlotClicked, int32 /*SlotId*/);

/**
 * 캐릭터 선택 화면의 슬롯 하나.
 *
 * 캐릭터 하나의 요약을 보여 주고, 눌리면 자기 슬롯 번호를 알린다.
 */
UCLASS()
class P1_API UP1CharacterSlotWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    //~ End UUserWidget Interface

    //~ Character Slot
public:
    /** 클릭 알림에 실을 슬롯 번호를 정한다. */
    void SetSlotId(int32 InSlotId);

    void ShowCharacter(const FP1LobbyCharacter& Character);

    /** 캐릭터가 없는 슬롯으로 되돌린다. */
    void Clear();

    void SetHighlighted(bool bHighlighted);
    bool HasCharacter() const;

    FOnCharacterSlotClicked OnSlotClicked;

    /** 화면에 보여 줄 직업 이름. 이름이 없는 직업이면 대체 이름. */
    static FString GetClassDisplayName(EP1CharacterClass CharacterClass);

private:
    UFUNCTION()
    void OnSlotButtonClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> SlotButton0;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Info_txt;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> Name_txt;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> Highlight_img;

    int32 SlotId = -1;

    /** 캐릭터가 없으면 -1. */
    int64 CharacterId = -1;
};
