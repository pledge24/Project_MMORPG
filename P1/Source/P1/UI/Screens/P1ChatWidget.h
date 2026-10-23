#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Types/SlateEnums.h"
#include "P1ChatWidget.generated.h"

class UEditableText;
class UPanelWidget;
class UScrollBox;
class UTextBlock;
class UWidget;
class UP1ChatLineWidget;
class UP1ChatSubsystem;

/**
 * 게임 화면 왼쪽 아래의 채팅창.
 *
 * 최근 채팅을 늘 보여 주고, 입력칸을 열면 지난 채팅을 스크롤해 읽거나 플레이어가 쓴 문장을 Enter로 보낸다.
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
    /** 채팅 서브시스템이 보관한 줄을 모두 줄 위젯에 채우고, 맨 아래를 보고 있었으면 맨 아래로 스크롤한다. */
    void RefreshLines();

    /** 스크롤이 맨 아래에 있으면 true. 맨 아래에서 이만큼(슬레이트 단위) 안이면 맨 아래로 본다. */
    bool IsScrolledToEnd() const;

    static constexpr float SCROLL_END_TOLERANCE = 4.f;

    UPROPERTY(EditDefaultsOnly, Category = "Chat")
    TSubclassOf<UP1ChatLineWidget> LineWidgetClass;

    /** 줄 영역의 스크롤. 줄이 영역보다 많거나 길면 스크롤한다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UScrollBox> LinesScroll;

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

    //~ 공통
private:
    /** 이 위젯을 만든 게임 인스턴스의 채팅 서브시스템. 없으면 nullptr. */
    UP1ChatSubsystem* GetChat() const;
};
