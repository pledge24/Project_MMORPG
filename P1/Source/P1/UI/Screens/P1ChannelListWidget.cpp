#include "UI/Screens/P1ChannelListWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Game/Channel/P1ChannelSubsystem.h"
#include "UI/P1ScreenSubsystem.h"
#include "UI/Screens/P1ChannelRowWidget.h"
#include "Utils/LogCategory.h"

void UP1ChannelListWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    CloseButton->OnClicked.AddDynamic(this, &UP1ChannelListWidget::OnCloseButtonClicked);
}

void UP1ChannelListWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 지금 목록으로 채우고 이후 바뀌는 목록과 채널을 구독.
    if (UP1ChannelSubsystem* Channels = GetChannels())
    {
        Channels->OnChannelListChanged.AddUObject(this, &UP1ChannelListWidget::RefreshRows);
        Channels->OnChannelChanged.AddUObject(this, &UP1ChannelListWidget::HandleChannelChanged);
        Channels->OnChannelChangeFailed.AddUObject(this, &UP1ChannelListWidget::HandleChangeFailed);
    }

    RefreshRows();
}

void UP1ChannelListWidget::NativeDestruct()
{
    if (UP1ChannelSubsystem* Channels = GetChannels())
        Channels->RemoveListener(this);

    Super::NativeDestruct();
}

void UP1ChannelListWidget::RefreshRows()
{
    NoticeText->SetText(FText::GetEmpty());

    const UP1ChannelSubsystem* Channels = GetChannels();
    if (Channels == nullptr)
    {
        UE_LOG(LogP1UI, Warning, TEXT("채널 목록 창이 채널 서브시스템을 찾지 못해 목록을 그리지 않는다"));
        return;
    }

    const int32 CurrentChannel = Channels->GetChannel();
    CurrentChannelText->SetText(FText::FromString(FString::Printf(TEXT("지금 채널: %d채널"), CurrentChannel)));

    if (RowWidgetClass == nullptr)
    {
        UE_LOG(LogP1UI, Warning, TEXT("채널 목록 창의 Row Widget Class가 비어 있어 채널 줄을 그리지 않는다: %s"), *GetClass()->GetPathName());
        return;
    }

    const TArray<FP1ChannelEntry>& Entries = Channels->GetChannels();
    while (RowWidgets.Num() < Entries.Num())
    {
        UP1ChannelRowWidget* RowWidget = CreateWidget<UP1ChannelRowWidget>(this, RowWidgetClass);
        if (RowWidget == nullptr)
            return;

        RowWidget->OnMoveClicked.BindUObject(this, &UP1ChannelListWidget::HandleMoveClicked);
        RowsBox->AddChild(RowWidget);
        RowWidgets.Add(RowWidget);
    }

    for (int32 Index = 0; Index < RowWidgets.Num(); ++Index)
    {
        if (Index < Entries.Num())
        {
            RowWidgets[Index]->ShowChannel(Entries[Index], Entries[Index].Channel == CurrentChannel);
            RowWidgets[Index]->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            RowWidgets[Index]->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UP1ChannelListWidget::HandleMoveClicked(int32 Channel)
{
    if (UP1ChannelSubsystem* Channels = GetChannels())
        Channels->RequestChangeChannel(Channel);
}

void UP1ChannelListWidget::HandleChannelChanged(int32 Channel)
{
    // 닫힌 창은 다음에 열 때 다시 그리지만, 지금 채널 문구가 낡은 채로 한 번 보이지 않도록 여기서도 그린다.
    RefreshRows();
    CloseSelf();
}

void UP1ChannelListWidget::HandleChangeFailed(const FText& Message)
{
    NoticeText->SetText(Message);
}

void UP1ChannelListWidget::OnCloseButtonClicked()
{
    CloseSelf();
}

void UP1ChannelListWidget::CloseSelf()
{
    if (UP1ScreenSubsystem* Screens = ULocalPlayer::GetSubsystem<UP1ScreenSubsystem>(GetOwningLocalPlayer()))
        Screens->CloseWindow(EP1WidgetType::WIDGET_CHANNEL_LIST);
}

UP1ChannelSubsystem* UP1ChannelListWidget::GetChannels() const
{
    const UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UP1ChannelSubsystem>() : nullptr;
}
