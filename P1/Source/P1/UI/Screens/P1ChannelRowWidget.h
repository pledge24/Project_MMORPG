#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1ChannelRowWidget.generated.h"

class UButton;
class UTextBlock;
struct FP1ChannelEntry;

DECLARE_DELEGATE_OneParam(FOnChannelRowMoveClicked, int32 /*Channel*/);

/**
 * 채널 목록 창의 채널 한 줄.
 *
 * 채널 번호와 인원을 보여 주고, 이동 버튼이 눌리면 자기 채널 번호를 알린다.
 * 지금 있는 채널이면 이동 버튼을 막는다.
 */
UCLASS()
class P1_API UP1ChannelRowWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    //~ End UUserWidget Interface

    //~ Channel Row
public:
    void ShowChannel(const FP1ChannelEntry& Entry, bool bCurrentChannel);

    FOnChannelRowMoveClicked OnMoveClicked;

private:
    UFUNCTION()
    void OnMoveButtonClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ChannelText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PlayerCountText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> MoveButton;

    int32 Channel = 0;
};
