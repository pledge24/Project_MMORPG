#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Lobby/P1CharacterClass.h"
#include "P1CharacterSlotWidget.generated.h"

class UButton;
class UTextBlock;
struct FP1LobbyCharacter;

DECLARE_DELEGATE_OneParam(FOnCharacterSlotClicked, int32 /*SlotId*/);

/**
 * 캐릭터 선택 화면의 슬롯 하나.
 *
 * 캐릭터 하나의 요약을 보여 주고, 눌리면 자기 슬롯 번호를 알린다.
 * 상태(캐릭터 있음, 고름, 빈 슬롯)와 직업마다 WBP에 따로 그려 둔 배경과 문장을 골라 보인다.
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

    void SetHighlighted(bool bInHighlighted);
    bool HasCharacter() const;

    FOnCharacterSlotClicked OnSlotClicked;

    /** 화면에 보여 줄 직업 이름. 이름이 없는 직업이면 대체 이름. */
    static FString GetClassDisplayName(EP1CharacterClass InCharacterClass);

private:
    /** 캐릭터 유무와 고름 여부에 맞는 배경과 내용을 보인다. */
    void ApplyState();

    UFUNCTION()
    void OnSlotButtonClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> SlotButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> FilledBackground;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> SelectedBackground;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> EmptyBackground;

    /** 캐릭터가 있을 때 보이는 문장 묶음. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> CharacterContent;

    /** 빈 슬롯일 때 보이는 문장 묶음. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> EmptyContent;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> WarriorEmblem;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> MageEmblem;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> NameText;

    /** 「직업 · Lv. 레벨」 */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> InfoText;

    int32 SlotId = -1;

    /** 캐릭터가 없으면 -1. */
    int64 CharacterId = -1;

    EP1CharacterClass CharacterClass = EP1CharacterClass::None;

    bool bHighlighted = false;
};
