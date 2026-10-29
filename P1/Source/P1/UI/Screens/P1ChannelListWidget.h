#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1ChannelListWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class UP1ChannelRowWidget;
class UP1ChannelSubsystem;

/**
 * 같은 맵의 채널 목록 창.
 *
 * 채널마다 번호와 인원을 보여 주고, 다른 채널로 옮기는 요청을 보내는 창구가 된다.
 * 목록은 채널 서브시스템이 받은 사본이고, 창을 여는 쪽이 목록을 새로 요청한다.
 */
UCLASS()
class P1_API UP1ChannelListWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    //~ End UUserWidget Interface

    //~ Channel List
private:
    /** 채널 서브시스템이 보관한 목록으로 줄 위젯을 채운다. 지금 채널의 줄은 이동 버튼을 막는다. */
    void RefreshRows();

    void HandleMoveClicked(int32 Channel);

    /** 채널을 옮겼으면 창을 닫는다. 새 채널의 인원은 다음에 창을 열 때 다시 받는다. */
    void HandleChannelChanged(int32 Channel);

    void HandleChangeFailed(const FText& Message);

    UFUNCTION()
    void OnCloseButtonClicked();

    void CloseSelf();

    UP1ChannelSubsystem* GetChannels() const;

    UPROPERTY(EditDefaultsOnly, Category = "Channel")
    TSubclassOf<UP1ChannelRowWidget> RowWidgetClass;

    /** 줄 위젯을 채널 순서로 쌓는 패널. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> RowsBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> CurrentChannelText;

    /** 채널 변경이 거절된 사유. 창을 다시 그리면 지운다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> NoticeText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CloseButton;

    /** 만든 줄 위젯. 채널 수가 줄면 숨기고 다시 쓴다. */
    UPROPERTY()
    TArray<TObjectPtr<UP1ChannelRowWidget>> RowWidgets;
};
