#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1CharacterSlotWidget.generated.h"

class UButton;
class UEditableTextBox;
class UImage;
class UTextBlock;
struct FP1CharacterOverview;

DECLARE_DELEGATE_OneParam(FOnCharacterSlotClicked, int32 /*SlotId*/);

/** 캐릭터 선택 화면의 슬롯 하나다. 캐릭터 요약을 보여 주고, 눌리면 로그인 메뉴에 슬롯 번호를 알린다. */
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
    /** 로그인 메뉴가 슬롯 목록의 순서대로 매긴다. 클릭 알림에 실린다. */
    void SetSlotId(int32 InSlotId);

    void ShowCharacter(const FP1CharacterOverview& Character);

    /** 캐릭터가 없는 슬롯으로 되돌린다. */
    void Clear();

    void SetHighlighted(bool bHighlighted);
    bool HasCharacter() const;

    FOnCharacterSlotClicked OnSlotClicked;

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

    /** 캐릭터가 없으면 -1이다. */
    int64 CharacterId = -1;
};
