#include "UI/Screens/P1ChatWidget.h"
#include "UI/Screens/P1ChatLineWidget.h"
#include "UI/P1ScreenSubsystem.h"
#include "Components/EditableText.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Game/Chat/P1ChatLimits.h"
#include "Game/Chat/P1ChatSubsystem.h"
#include "TimerManager.h"

namespace
{
    /** 이 위젯을 소유한 로컬 플레이어의 화면 서브시스템. 없으면 nullptr. */
    UP1ScreenSubsystem* GetScreens(const UUserWidget& Widget)
    {
        const ULocalPlayer* LocalPlayer = Widget.GetOwningLocalPlayer();
        return LocalPlayer ? LocalPlayer->GetSubsystem<UP1ScreenSubsystem>() : nullptr;
    }
}

void UP1ChatWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 입력칸은 Enter로만 닫는다. 보낸 뒤에도 포커스를 입력칸이 놓지 않고, Esc가 쓴 글자를 되돌리지 않는다.
    InputText->SetClearKeyboardFocusOnCommit(false);
    InputText->SetRevertTextOnEscape(false);
    InputText->OnTextChanged.AddDynamic(this, &UP1ChatWidget::HandleInputChanged);
    InputText->OnTextCommitted.AddDynamic(this, &UP1ChatWidget::HandleInputCommitted);
}

void UP1ChatWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 지금 줄로 채우고 이후 바뀌는 줄을 구독.
    if (UP1ChatSubsystem* Chat = GetChat())
        Chat->OnChatLogChanged.AddUObject(this, &UP1ChatWidget::RefreshLines);

    RefreshLines();
    HideInput();
}

void UP1ChatWidget::NativeDestruct()
{
    if (UP1ChatSubsystem* Chat = GetChat())
        Chat->RemoveListener(this);

    Super::NativeDestruct();
}

void UP1ChatWidget::RefreshLines()
{
    const UP1ChatSubsystem* Chat = GetChat();
    if (Chat == nullptr || LineWidgetClass == nullptr)
        return;

    const TArray<FP1ChatLine>& Lines = Chat->GetLines();
    const int32 FirstIndex = FMath::Max(0, Lines.Num() - VisibleLineCount);
    const int32 ShownCount = Lines.Num() - FirstIndex;

    while (LineWidgets.Num() < ShownCount)
    {
        UP1ChatLineWidget* LineWidget = CreateWidget<UP1ChatLineWidget>(this, LineWidgetClass);
        if (LineWidget == nullptr)
            return;

        LinesBox->AddChild(LineWidget);
        LineWidgets.Add(LineWidget);
    }

    for (int32 Index = 0; Index < LineWidgets.Num(); ++Index)
    {
        if (Index < ShownCount)
        {
            LineWidgets[Index]->SetLine(Lines[FirstIndex + Index]);
            LineWidgets[Index]->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            LineWidgets[Index]->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

UP1ChatSubsystem* UP1ChatWidget::GetChat() const
{
    const UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UP1ChatSubsystem>() : nullptr;
}

void UP1ChatWidget::ShowInput()
{
    InputText->SetText(FText::GetEmpty());
    ShowInputLength(0);

    Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
    InputBox->SetVisibility(ESlateVisibility::Visible);
    SendHint->SetVisibility(ESlateVisibility::HitTestInvisible);
    OpenHint->SetVisibility(ESlateVisibility::Collapsed);
}

void UP1ChatWidget::HideInput()
{
    InputText->SetText(FText::GetEmpty());

    Panel->SetVisibility(ESlateVisibility::Hidden);
    InputBox->SetVisibility(ESlateVisibility::Collapsed);
    SendHint->SetVisibility(ESlateVisibility::Collapsed);
    OpenHint->SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedPtr<SWidget> UP1ChatWidget::GetInputFocusTarget() const
{
    return InputText->GetCachedWidget();
}

void UP1ChatWidget::HandleInputChanged(const FText& Text)
{
    // 서버가 길이를 넘는 메시지를 거절하므로 입력칸에서 먼저 자른다. 자르면 이 함수가 잘린 글자로 다시 불린다.
    const FString Input = Text.ToString();
    if (Input.Len() > P1ChatLimits::MAX_MESSAGE_LENGTH)
    {
        InputText->SetText(FText::FromString(Input.Left(P1ChatLimits::MAX_MESSAGE_LENGTH)));
        return;
    }

    ShowInputLength(Input.Len());
}

void UP1ChatWidget::HandleInputCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    UP1ScreenSubsystem* Screens = GetScreens(*this);
    if (Screens == nullptr || Screens->IsChatInputOpen() == false)
        return;

    if (CommitMethod == ETextCommit::OnEnter)
    {
        // 비었거나 공백만 있으면 보내지 않고 입력칸만 닫는다.
        if (UP1ChatSubsystem* Chat = GetChat())
            Chat->SendChat(Text.ToString());

        Screens->CloseChatInput();
        return;
    }

    // 입력칸은 Enter로만 닫는다. 입력 중에 화면을 눌러 포커스가 빠지면 다음 프레임에 입력 모드를 다시 맞춘다.
    // 그 사이 사망 화면이나 게임 메뉴가 떴으면 입력 모드가 그 화면에 포커스를 준다. 포커스가 바뀌는 중에는 다시 옮기지 않는다.
    if (CommitMethod == ETextCommit::OnUserMovedFocus)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
                {
                    UP1ScreenSubsystem* ScreensNow = GetScreens(*this);
                    if (ScreensNow && ScreensNow->IsChatInputOpen())
                        ScreensNow->RefreshInputMode();
                }));
        }
    }
}

void UP1ChatWidget::ShowInputLength(int32 Length)
{
    InputLengthText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Length, P1ChatLimits::MAX_MESSAGE_LENGTH)));
}
