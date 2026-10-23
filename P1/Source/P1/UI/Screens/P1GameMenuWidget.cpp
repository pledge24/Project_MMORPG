#include "UI/Screens/P1GameMenuWidget.h"
#include "Components/Button.h"
#include "Core/P1EntryFlowSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/P1ScreenSubsystem.h"

namespace
{
    UP1EntryFlowSubsystem* GetEntryFlow(const UUserWidget& Widget)
    {
        UGameInstance* GameInstance = Widget.GetGameInstance();
        return GameInstance ? GameInstance->GetSubsystem<UP1EntryFlowSubsystem>() : nullptr;
    }
}

void UP1GameMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 입력을 이 메뉴만 받으려면 키 포커스를 받을 수 있어야 한다.
    SetIsFocusable(true);

    CharacterSelectButton->OnClicked.AddDynamic(this, &UP1GameMenuWidget::OnCharacterSelectButtonClicked);
    WorldSelectButton->OnClicked.AddDynamic(this, &UP1GameMenuWidget::OnWorldSelectButtonClicked);
    QuitButton->OnClicked.AddDynamic(this, &UP1GameMenuWidget::OnQuitButtonClicked);
    ResumeButton->OnClicked.AddDynamic(this, &UP1GameMenuWidget::OnResumeButtonClicked);
}

FReply UP1GameMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // 메뉴가 열려 있으면 입력이 플레이어 컨트롤러에 가지 않으므로, 여는 키로 닫는 일은 메뉴가 받는다.
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        CloseMenu();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UP1GameMenuWidget::OnCharacterSelectButtonClicked()
{
    CloseMenu();

    if (UP1EntryFlowSubsystem* EntryFlow = GetEntryFlow(*this))
        EntryFlow->ReturnToLobby();
}

void UP1GameMenuWidget::OnWorldSelectButtonClicked()
{
    CloseMenu();

    if (UP1EntryFlowSubsystem* EntryFlow = GetEntryFlow(*this))
        EntryFlow->ReturnToWorldSelect();
}

void UP1GameMenuWidget::OnQuitButtonClicked()
{
    // 연결은 게임이 꺼지면서 닫히고, 서버는 접속 종료 경로로 진행을 저장한다.
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UP1GameMenuWidget::OnResumeButtonClicked()
{
    CloseMenu();
}

void UP1GameMenuWidget::CloseMenu()
{
    if (UP1ScreenSubsystem* Screens = ULocalPlayer::GetSubsystem<UP1ScreenSubsystem>(GetOwningLocalPlayer()))
        Screens->CloseGameMenu();
}
