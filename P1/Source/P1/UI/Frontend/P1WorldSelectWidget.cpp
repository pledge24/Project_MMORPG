#include "UI/Frontend/P1WorldSelectWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "Auth/P1AuthClient.h"
#include "Core/P1EntryFlowSubsystem.h"
#include "UI/Frontend/P1WorldRowWidget.h"
#include "Utils/LogCategory.h"

void UP1WorldSelectWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    ConnectButton->OnClicked.AddDynamic(this, &UP1WorldSelectWidget::OnConnectButtonClicked);
    LogoutButton->OnClicked.AddDynamic(this, &UP1WorldSelectWidget::OnLogoutButtonClicked);
}

void UP1WorldSelectWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 응답 구독. 떼는 것은 NativeDestruct가 한다.
    if (UP1AuthClient* AuthClient = GetAuthClient())
        AuthClient->OnWorldsChanged.AddUObject(this, &UP1WorldSelectWidget::Refresh);
}

void UP1WorldSelectWidget::NativeDestruct()
{
    Close();

    // 서브시스템은 레벨보다 오래 산다. 떼지 않으면 로그인 맵을 다시 열 때마다 죽은 위젯의 항목이 쌓인다.
    if (UP1AuthClient* AuthClient = GetAuthClient())
        AuthClient->RemoveListener(this);

    Super::NativeDestruct();
}

void UP1WorldSelectWidget::Open()
{
    DescriptionText->SetText(FText::GetEmpty());
    Refresh();

    // 게임 서버가 꺼진 월드는 인증 서버의 목록에서 빠진다. 화면에 있는 동안 다시 받아야 그것이 보인다.
    GetWorld()->GetTimerManager().SetTimer(RefreshTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        if (UP1AuthClient* AuthClient = GetAuthClient())
            AuthClient->RequestWorlds();
    }), REFRESH_SECONDS, true);
}

void UP1WorldSelectWidget::Close()
{
    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(RefreshTimer);
}

void UP1WorldSelectWidget::ShowNotice(const FString& Notice)
{
    DescriptionText->SetText(FText::FromString(Notice));
}

UP1AuthClient* UP1WorldSelectWidget::GetAuthClient() const
{
    UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UP1AuthClient>() : nullptr;
}

UP1EntryFlowSubsystem* UP1WorldSelectWidget::GetEntryFlow() const
{
    UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UP1EntryFlowSubsystem>() : nullptr;
}

void UP1WorldSelectWidget::OnConnectButtonClicked()
{
    if (SelectedWorldId == 0)
    {
        ShowNotice(TEXT("접속할 월드를 선택하지 않았습니다."));
        return;
    }

    DescriptionText->SetText(FText::GetEmpty());

    // 혼잡한 월드도 고를 수 있다. 실패하면 접속 흐름이 사유를 이 화면에 넘겨준다.
    if (UP1EntryFlowSubsystem* EntryFlow = GetEntryFlow())
        EntryFlow->SelectWorld(SelectedWorldId);
}

void UP1WorldSelectWidget::OnLogoutButtonClicked()
{
    if (UP1EntryFlowSubsystem* EntryFlow = GetEntryFlow())
        EntryFlow->Logout();
}

void UP1WorldSelectWidget::Refresh()
{
    const UP1AuthClient* AuthClient = GetAuthClient();
    const TArray<FP1World> NoWorlds;
    const TArray<FP1World>& Worlds = AuthClient ? AuthClient->GetWorlds() : NoWorlds;

    if (WorldRowClass == nullptr && Worlds.Num() > 0)
    {
        UE_LOG(LogP1UI, Error, TEXT("월드 선택 화면에 월드 줄의 WBP(WorldRowClass)가 없어 월드를 보여 주지 못합니다"));
        return;
    }

    // 모자란 줄만 새로 만든다. 주기마다 다시 그리므로 매번 만들면 위젯이 쌓인다.
    while (WorldRows.Num() < Worlds.Num())
    {
        UP1WorldRowWidget* Row = CreateWidget<UP1WorldRowWidget>(this, WorldRowClass);
        Row->OnRowClicked.BindUObject(this, &UP1WorldSelectWidget::SelectWorld);
        WorldList->AddChild(Row);
        WorldRows.Add(Row);
    }

    bool bSelectedWorldListed = false;
    for (int32 i = 0; i < WorldRows.Num(); i++)
    {
        if (Worlds.IsValidIndex(i) == false)
        {
            WorldRows[i]->SetVisibility(ESlateVisibility::Collapsed);
            continue;
        }

        WorldRows[i]->ShowWorld(Worlds[i]);
        WorldRows[i]->SetVisibility(ESlateVisibility::Visible);
        bSelectedWorldListed |= Worlds[i].WorldId == SelectedWorldId;
    }

    EmptyNotice->SetVisibility(Worlds.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

    // 고른 월드가 꺼져 목록에서 빠졌으면 그 월드로 접속하지 않게 선택을 푼다.
    SelectWorld(bSelectedWorldListed ? SelectedWorldId : 0);
}

void UP1WorldSelectWidget::SelectWorld(int32 WorldId)
{
    SelectedWorldId = WorldId;

    for (UP1WorldRowWidget* Row : WorldRows)
        Row->SetHighlighted(WorldId != 0 && Row->GetWorldId() == WorldId);
}
