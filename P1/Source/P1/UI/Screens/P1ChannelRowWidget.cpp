#include "UI/Screens/P1ChannelRowWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Game/Channel/P1ChannelEntry.h"

void UP1ChannelRowWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    MoveButton->OnClicked.AddDynamic(this, &UP1ChannelRowWidget::OnMoveButtonClicked);
}

void UP1ChannelRowWidget::ShowChannel(const FP1ChannelEntry& Entry, bool bCurrentChannel)
{
    Channel = Entry.Channel;
    ChannelText->SetText(FText::FromString(FString::Printf(TEXT("%d채널"), Entry.Channel)));
    PlayerCountText->SetText(FText::FromString(FString::Printf(TEXT("%d명"), Entry.PlayerCount)));

    // 서버는 지금 채널로 옮기는 요청을 거절한다.
    MoveButton->SetIsEnabled(bCurrentChannel == false);
}

void UP1ChannelRowWidget::OnMoveButtonClicked()
{
    OnMoveClicked.ExecuteIfBound(Channel);
}
