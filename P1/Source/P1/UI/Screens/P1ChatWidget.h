#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Types/SlateEnums.h"
#include "P1ChatWidget.generated.h"

class UEditableText;
class UPanelWidget;
class UTextBlock;
class UWidget;
class UP1ChatLineWidget;
class UP1ChatSubsystem;

/**
 * 게임 화면 왼쪽 아래의 채팅창.
 *
 * 최근 채팅을 늘 보여 주고, 입력칸을 열면 플레이어가 쓴 문장을 Enter로 보낸다.
 * 입력칸을 열고 닫는 것과 입력 모드는 화면 서브시스템이 정한다.
 */
UCLASS()
class P1_API UP1ChatWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    //~ End UUserWidget Interface

    //~ Lines
private:
    /** 채팅 서브시스템의 최근 줄 중 화면에 보일 수만큼을 줄 위젯에 채운다. */
    void RefreshLines();

    /** 이 위젯을 만든 게임 인스턴스의 채팅 서브시스템. 없으면 nullptr. */
    UP1ChatSubsystem* GetChat() const;

    /** 화면에 보이는 줄의 수. 그보다 오래된 줄은 채팅 서브시스템에만 남는다. */
    UPROPERTY(EditDefaultsOnly, Category = "Chat", meta = (ClampMin = "1"))
    int32 VisibleLineCount = 8;

    UPROPERTY(EditDefaultsOnly, Category = "Chat")
    TSubclassOf<UP1ChatLineWidget> LineWidgetClass;

    /** 줄 위젯을 아래로 쌓는 패널. 오래된 줄이 위에 있다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> LinesBox;

    /** 만든 줄 위젯. 보일 줄이 줄면 숨기고 다시 쓴다. */
    UPROPERTY()
    TArray<TObjectPtr<UP1ChatLineWidget>> LineWidgets;

    //~ Input
public:
    /** 입력칸과 그 배경을 보이고 입력칸에 쓴 글자를 비운다. 입력 포커스는 화면 서브시스템이 준다. */
    void ShowInput();

    /** 입력칸과 그 배경을 숨기고 평소 모습으로 돌아간다. */
    void HideInput();

    /** 입력 중일 때 키보드를 받을 위젯. */
    TSharedPtr<SWidget> GetInputFocusTarget() const;

private:
    UFUNCTION()
    void HandleInputChanged(const FText& Text);

    UFUNCTION()
    void HandleInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    void ShowInputLength(int32 Length);

    /** 입력 중에만 보이는 채팅창 배경. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> Panel;

    /** 입력 중에만 보이는 입력칸 묶음. 입력칸과 글자 수를 담는다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> InputBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableText> InputText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> InputLengthText;

    /** 입력하지 않을 때 보이는 여는 키 안내. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> OpenHint;

    /** 입력 중에 보이는 보내는 키 안내. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> SendHint;
};
