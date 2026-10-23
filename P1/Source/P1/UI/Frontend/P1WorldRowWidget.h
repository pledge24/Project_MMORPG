#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Auth/P1WorldCongestion.h"
#include "P1WorldRowWidget.generated.h"

class UButton;
class UTextBlock;
struct FP1World;

DECLARE_DELEGATE_OneParam(FOnWorldRowClicked, int32 /*WorldId*/);

/**
 * 월드 선택 화면의 월드 한 줄.
 *
 * 월드 하나의 이름과 혼잡도를 보여 주고, 눌리면 자기 월드 번호를 알린다.
 * 고름 여부와 혼잡도마다 WBP에 따로 그려 둔 배경과 표시를 골라 보인다.
 */
UCLASS()
class P1_API UP1WorldRowWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    //~ End UUserWidget Interface

    //~ World Row
public:
    void ShowWorld(const FP1World& World);

    int32 GetWorldId() const { return WorldId; }

    void SetHighlighted(bool bInHighlighted);

    FOnWorldRowClicked OnRowClicked;

private:
    /** 고름 여부와 혼잡도에 맞는 배경과 표시를 보인다. */
    void ApplyState();

    UFUNCTION()
    void OnRowButtonClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RowButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> NormalBackground;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> SelectedBackground;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> NameText;

    /** 「원활」 표시. 보통, 혼잡 표시와 같은 자리에 겹쳐 두고 하나만 보인다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> SmoothBadge;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> NormalBadge;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> CrowdedBadge;

    int32 WorldId = 0;

    EP1WorldCongestion Congestion = EP1WorldCongestion::Smooth;

    bool bHighlighted = false;
};
