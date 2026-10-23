#include "UI/Frontend/P1LoginMenuWidget.h"
#include "Components/WidgetSwitcher.h"
#include "Core/P1EntryFlowSubsystem.h"
#include "Lobby/P1LobbySubsystem.h"
#include "UI/Frontend/P1CharacterCreateWidget.h"
#include "UI/Frontend/P1CharacterDeleteWidget.h"
#include "UI/Frontend/P1CharacterSelectWidget.h"
#include "UI/Frontend/P1LoginWidget.h"
#include "UI/Frontend/P1WorldSelectWidget.h"

void UP1LoginMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 화면이 보낸 요청으로 화면을 바꾼다.
    CharacterSelectScreen->OnCreateRequested.AddUObject(this, &UP1LoginMenuWidget::ShowCharacterCreate);
    CharacterSelectScreen->OnDeleteRequested.AddUObject(this, &UP1LoginMenuWidget::ShowCharacterDelete);
    CharacterCreateScreen->OnClosed.AddUObject(this, &UP1LoginMenuWidget::ShowCharacterSelect);
    CharacterDeleteScreen->OnClosed.AddUObject(this, &UP1LoginMenuWidget::ShowCharacterSelect);
}

void UP1LoginMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 로그인 화면에서 시작. 로그인 맵은 로그인 단계에서만 다시 열린다.
    SetActiveScreen(LoginScreen);

    // 응답 구독. 떼는 것은 NativeDestruct가 한다.
    UGameInstance* GameInstance = GetGameInstance();
    if (UP1LobbySubsystem* Lobby = GameInstance ? GameInstance->GetSubsystem<UP1LobbySubsystem>() : nullptr)
    {
        // 목록을 받거나 생성·삭제에 성공하면 캐릭터 선택 화면으로 돌아간다.
        Lobby->OnCharactersChanged.AddUObject(this, &UP1LoginMenuWidget::ShowCharacterSelect);
        Lobby->OnDeleteCharacterFailed.AddUObject(this, &UP1LoginMenuWidget::HandleDeleteCharacterFailed);
    }

    if (UP1EntryFlowSubsystem* EntryFlow = GameInstance ? GameInstance->GetSubsystem<UP1EntryFlowSubsystem>() : nullptr)
    {
        EntryFlow->OnPhaseChanged.AddUObject(this, &UP1LoginMenuWidget::ShowScreenForPhase);
        EntryFlow->OnNotice.AddUObject(this, &UP1LoginMenuWidget::ShowNotice);
    }
}

void UP1LoginMenuWidget::NativeDestruct()
{
    // 서브시스템은 레벨보다 오래 산다. 떼지 않으면 로그인 맵을 다시 열 때마다 죽은 위젯의 항목이 쌓인다.
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UP1LobbySubsystem* Lobby = GameInstance->GetSubsystem<UP1LobbySubsystem>())
            Lobby->RemoveListener(this);

        if (UP1EntryFlowSubsystem* EntryFlow = GameInstance->GetSubsystem<UP1EntryFlowSubsystem>())
            EntryFlow->RemoveListener(this);
    }

    Super::NativeDestruct();
}

void UP1LoginMenuWidget::ShowScreenForPhase(EP1EntryPhase Phase)
{
    switch (Phase)
    {
    case EP1EntryPhase::Login:
        SetActiveScreen(LoginScreen);
        break;
    case EP1EntryPhase::WorldSelect:
        SetActiveScreen(WorldSelectScreen);
        WorldSelectScreen->Open();
        break;
    default:
        break;
    }
}

void UP1LoginMenuWidget::ShowCharacterSelect()
{
    // 슬롯은 캐릭터 선택 화면이 로비의 목록 변경 알림을 받아 직접 다시 그린다.
    // 삭제의 성공과 실패, 취소 모두 이리로 오므로 삭제 확인 창도 여기서 닫힌다.
    SetActiveScreen(CharacterSelectScreen);
}

void UP1LoginMenuWidget::ShowCharacterCreate()
{
    CharacterCreateScreen->Reset();
    SetActiveScreen(CharacterCreateScreen);
}

void UP1LoginMenuWidget::ShowCharacterDelete(const FP1LobbyCharacter& Character)
{
    // 확인 창은 캐릭터 선택 화면 위에 덮는다. 화면 전환기 밖에 있어서 선택 화면이 그 아래에 보인다.
    CharacterDeleteScreen->SetCharacter(Character);
    CharacterDeleteScreen->SetVisibility(ESlateVisibility::Visible);
}

void UP1LoginMenuWidget::SetActiveScreen(UWidget* Screen)
{
    if (Screen != WorldSelectScreen)
        WorldSelectScreen->Close();

    CharacterDeleteScreen->SetVisibility(ESlateVisibility::Collapsed);
    ScreenSwitcher->SetActiveWidget(Screen);
}

void UP1LoginMenuWidget::ShowNotice(const FString& Notice)
{
    // 단계를 바꾸며 알릴 때는 단계 알림이 먼저 와서 그 단계의 화면이 이미 보인다.
    if (ScreenSwitcher->GetActiveWidget() == WorldSelectScreen)
        WorldSelectScreen->ShowNotice(Notice);
    else
        LoginScreen->ShowNotice(Notice);
}

void UP1LoginMenuWidget::HandleDeleteCharacterFailed(const FText& Message)
{
    ShowCharacterSelect();
}
