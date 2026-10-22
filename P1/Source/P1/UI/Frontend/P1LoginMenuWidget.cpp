#include "UI/Frontend/P1LoginMenuWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "TimerManager.h"
#include "Auth/P1AuthClient.h"
#include "Core/P1EntryFlowSubsystem.h"
#include "Lobby/P1LobbySubsystem.h"
#include "UI/Frontend/P1CharacterSlotWidget.h"
#include "Utils/LogCategory.h"

void UP1LoginMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 화면별 버튼 연결.
    LoginButton->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnLoginButtonClicked);
    RegisterButton->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnRegisterButtonClicked);

    CS_CreateBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnCreateButtonClicked);
    CS_DeleteBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnDeleteButtonClicked);
    CS_GameStartBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnGameStartButtonClicked);

    CC_WarriorBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnWarriorButtonClicked);
    CC_MagicianBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnMagicianButtonClicked);
    CC_CreateBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnCreateConfirmButtonClicked);
    CC_CancelBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnCreateCancelButtonClicked);

    CD_ConfirmButton->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnDeleteConfirmButtonClicked);
    CD_CancelButton->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnDeleteCancelButtonClicked);

    // 패널에 놓인 순서대로 슬롯 번호를 매긴다.
    for (UWidget* Child : Slots->GetAllChildren())
    {
        UP1CharacterSlotWidget* CharacterSlot = Cast<UP1CharacterSlotWidget>(Child);
        if (CharacterSlot == nullptr)
            continue;

        CharacterSlot->SetSlotId(CharacterSlots.Num());
        CharacterSlot->OnSlotClicked.BindUObject(this, &UP1LoginMenuWidget::SelectSlot);
        CharacterSlots.Add(CharacterSlot);
    }
}

void UP1LoginMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 로그인 화면에서 시작.
    WidgetSwitcher->SetActiveWidget(LoginScreen);

    // 응답 구독. 떼는 것은 NativeDestruct가 한다.
    if (UP1AuthClient* AuthClient = GetAuthClient())
        AuthClient->OnAuthResult.AddUObject(this, &UP1LoginMenuWidget::SetResultText);

    if (UP1LobbySubsystem* Lobby = GetLobby())
    {
        Lobby->OnCharactersChanged.AddUObject(this, &UP1LoginMenuWidget::ShowCharacters);
        Lobby->OnCharacterListFailed.AddUObject(this, &UP1LoginMenuWidget::ShowCharacterListFailed);
        Lobby->OnCreateCharacterFailed.AddUObject(this, &UP1LoginMenuWidget::ShowCreateCharacterFailed);
        Lobby->OnDeleteCharacterFailed.AddUObject(this, &UP1LoginMenuWidget::ShowDeleteCharacterFailed);
        Lobby->OnEnterGameFailed.AddUObject(this, &UP1LoginMenuWidget::ShowEnterGameFailed);
    }

    // 게임 서버와 연결이 끊겨 돌아왔으면 그 사유를 보여 주고, 이 화면에 있는 동안 생긴 사유도 받는다.
    UGameInstance* GameInstance = GetGameInstance();
    if (UP1EntryFlowSubsystem* EntryFlow = GameInstance ? GameInstance->GetSubsystem<UP1EntryFlowSubsystem>() : nullptr)
    {
        const FString Notice = EntryFlow->ConsumeLoginNotice();
        if (Notice.IsEmpty() == false)
            ShowLoginNotice(Notice);

        EntryFlow->OnLoginNotice.AddUObject(this, &UP1LoginMenuWidget::ShowLoginNotice);
    }
}

void UP1LoginMenuWidget::NativeDestruct()
{
    // 서브시스템은 레벨보다 오래 산다. 떼지 않으면 로그인 맵을 다시 열 때마다 죽은 위젯의 항목이 쌓인다.
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UP1AuthClient* AuthClient = GameInstance->GetSubsystem<UP1AuthClient>())
            AuthClient->RemoveListener(this);

        if (UP1LobbySubsystem* Lobby = GameInstance->GetSubsystem<UP1LobbySubsystem>())
            Lobby->RemoveListener(this);

        if (UP1EntryFlowSubsystem* EntryFlow = GameInstance->GetSubsystem<UP1EntryFlowSubsystem>())
            EntryFlow->RemoveListener(this);
    }

    Super::NativeDestruct();
}

UP1AuthClient* UP1LoginMenuWidget::GetAuthClient() const
{
    UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UP1AuthClient>() : nullptr;
}

void UP1LoginMenuWidget::SetResultText(bool bSuccess, const FString& Message)
{
    FLinearColor Color = bSuccess ? FLinearColor::White : FLinearColor::Red;

    ResultText->SetColorAndOpacity(FSlateColor(Color));
    ResultText->SetText(FText::FromString(Message));
}

void UP1LoginMenuWidget::ShowLoginNotice(const FString& Notice)
{
    SetResultText(false, Notice);
}

void UP1LoginMenuWidget::LockButtonBriefly(UButton* Button, FTimerHandle& LockTimer)
{
    // 잠긴 동안에는 버튼이 눌리지 않으므로 타이머가 겹쳐 걸리지 않는다.
    Button->SetIsEnabled(false);
    GetWorld()->GetTimerManager().SetTimer(LockTimer, FTimerDelegate::CreateWeakLambda(Button, [Button]()
    {
        Button->SetIsEnabled(true);
    }), BUTTON_LOCK_SECONDS, false);
}

void UP1LoginMenuWidget::OnLoginButtonClicked()
{
    if (UP1AuthClient* AuthClient = GetAuthClient())
    {
        AuthClient->RequestLogin(UsernameBox->GetText().ToString(), PasswordBox->GetText().ToString());
    }

    LockButtonBriefly(LoginButton, LoginButtonLockTimer);
}

void UP1LoginMenuWidget::OnRegisterButtonClicked()
{
    if (UP1AuthClient* AuthClient = GetAuthClient())
    {
        AuthClient->RequestRegister(UsernameBox->GetText().ToString(), PasswordBox->GetText().ToString());
    }

    LockButtonBriefly(RegisterButton, RegisterButtonLockTimer);
}

UP1LobbySubsystem* UP1LoginMenuWidget::GetLobby() const
{
    UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UP1LobbySubsystem>() : nullptr;
}

void UP1LoginMenuWidget::ShowCharacters()
{
    UP1LobbySubsystem* Lobby = GetLobby();
    if (Lobby == nullptr)
        return;

    // DisplayCharacters가 보이는 슬롯 수까지만 채우므로 슬롯 수를 먼저 적용한다.
    ApplyCharacterSlotCount();
    DisplayCharacters();
}

void UP1LoginMenuWidget::ShowCharacterListFailed()
{
    SetResultText(false, TEXT("캐릭터 목록을 불러오지 못했습니다."));
}

void UP1LoginMenuWidget::ApplyCharacterSlotCount()
{
    const UP1LobbySubsystem* Lobby = GetLobby();
    const int32 CharacterSlotCount = Lobby ? Lobby->GetCharacterSlotCount() : 0;

    // 위젯이 모자라면 넘친 캐릭터를 보여 줄 자리가 없다. 0 이하는 이 필드를 모르는 서버가 보낸 기본값이다.
    if (CharacterSlots.Num() < CharacterSlotCount || CharacterSlotCount <= 0)
    {
        UE_LOG(LogP1UI, Warning, TEXT("슬롯 위젯 수와 서버의 슬롯 수가 맞지 않습니다. 위젯 수: %d, 서버 슬롯 수: %d"), CharacterSlots.Num(), CharacterSlotCount);
    }

    // 보일 슬롯은 숨겼던 것만 되살린다. 디자이너가 정한 표시 상태를 덮어쓰지 않는다.
    const int32 VisibleCount = GetVisibleSlotCount();
    for (int32 i = 0; i < CharacterSlots.Num(); i++)
    {
        if (i >= VisibleCount)
            CharacterSlots[i]->SetVisibility(ESlateVisibility::Collapsed);
        else if (CharacterSlots[i]->GetVisibility() == ESlateVisibility::Collapsed)
            CharacterSlots[i]->SetVisibility(ESlateVisibility::Visible);
    }

    if (SelectedSlotIndex >= VisibleCount)
        SelectSlot(-1);
}

int32 UP1LoginMenuWidget::GetVisibleSlotCount() const
{
    const UP1LobbySubsystem* Lobby = GetLobby();
    return Lobby ? FMath::Min(Lobby->GetCharacterSlotCount(), CharacterSlots.Num()) : 0;
}

void UP1LoginMenuWidget::DisplayCharacters()
{
    for (UP1CharacterSlotWidget* CharacterSlot : CharacterSlots)
        CharacterSlot->Clear();

    // 보이는 슬롯보다 많은 캐릭터는 보이지 않는다.
    if (UP1LobbySubsystem* Lobby = GetLobby())
    {
        const TArray<FP1LobbyCharacter>& Characters = Lobby->GetCharacters();
        const int32 VisibleCount = FMath::Min(Characters.Num(), GetVisibleSlotCount());
        for (int32 i = 0; i < VisibleCount; i++)
            CharacterSlots[i]->ShowCharacter(Characters[i]);
    }

    WidgetSwitcher->SetActiveWidget(CharacterSelectScreen);
}

void UP1LoginMenuWidget::SelectSlot(int32 SlotIndex)
{
    SelectedSlotIndex = SlotIndex;

    for (int32 i = 0; i < CharacterSlots.Num(); i++)
        CharacterSlots[i]->SetHighlighted(i == SlotIndex);
}

bool UP1LoginMenuWidget::IsSelectedSlotOccupied() const
{
    return CharacterSlots.IsValidIndex(SelectedSlotIndex) && CharacterSlots[SelectedSlotIndex]->HasCharacter();
}

int64 UP1LoginMenuWidget::GetSelectedCharacterId() const
{
    // 슬롯 번호가 곧 목록의 순번이다. DisplayCharacters가 목록을 앞 슬롯부터 채운다.
    const UP1LobbySubsystem* Lobby = GetLobby();
    if (Lobby == nullptr || Lobby->GetCharacters().IsValidIndex(SelectedSlotIndex) == false)
        return -1;

    return Lobby->GetCharacters()[SelectedSlotIndex].CharacterId;
}

void UP1LoginMenuWidget::ShowDescription(const FText& Message)
{
    CS_Description->SetText(Message);

    // 같은 핸들로 다시 걸면 이전 타이머가 취소되고 처음부터 센다.
    GetWorld()->GetTimerManager().SetTimer(DescriptionTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        CS_Description->SetText(FText::GetEmpty());
    }), DESCRIPTION_SECONDS, false);
}

void UP1LoginMenuWidget::ShowEnterGameFailed()
{
    UTextBlock* TargetText = CS_ResultText ? CS_ResultText.Get() : ResultText.Get();
    TargetText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
    TargetText->SetText(FText::FromString(TEXT("게임에 입장하지 못했습니다. 캐릭터 데이터를 확인해 주세요.")));
    TargetText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UP1LoginMenuWidget::OnCreateButtonClicked()
{
    // 서버도 생성 요청을 같은 슬롯 수로 막는다. 여기서 먼저 막는 것은 생성 화면을 열지 않기 위해서다.
    UP1LobbySubsystem* Lobby = GetLobby();
    if (Lobby == nullptr || Lobby->GetCharacters().Num() >= GetVisibleSlotCount())
    {
        ShowDescription(FText::FromString(TEXT("캐릭터가 꽉 차있습니다!")));
        return;
    }

    ResetCharacterCreateScreen();
    WidgetSwitcher->SetActiveWidget(CharacterCreateScreen);
}

void UP1LoginMenuWidget::OnDeleteButtonClicked()
{
    // 확인 화면에는 문구를 띄울 자리가 없으므로 빈 칸은 여기서 거른다.
    if (IsSelectedSlotOccupied())
    {
        WidgetSwitcher->SetActiveWidget(CharacterDeleteScreen);
        return;
    }

    ShowDescription(FText::FromString(TEXT("삭제할 캐릭터를 선택하지 않았습니다.")));
}

void UP1LoginMenuWidget::OnGameStartButtonClicked()
{
    const int64 CharacterId = GetSelectedCharacterId();
    if (IsSelectedSlotOccupied() && CharacterId != -1)
    {
        if (UP1LobbySubsystem* Lobby = GetLobby())
            Lobby->SelectCharacter(CharacterId);
        return;
    }

    ShowDescription(FText::FromString(TEXT("캐릭터를 선택하지 않았습니다.")));
}

void UP1LoginMenuWidget::ResetCharacterCreateScreen()
{
    SelectedClass = EP1CharacterClass::None;
    SelectedClassText->SetText(FText::FromString(TEXT("선택 안 함")));
    CC_DescriptionText->SetText(FText::GetEmpty());
    CC_CharacterNameText->SetText(FText::GetEmpty());
}

void UP1LoginMenuWidget::SelectClass(EP1CharacterClass CharacterClass)
{
    SelectedClass = CharacterClass;
    SelectedClassText->SetText(FText::FromString(UP1CharacterSlotWidget::GetClassDisplayName(CharacterClass)));
}

void UP1LoginMenuWidget::ShowCreateCharacterFailed(const FString& Cause)
{
    CC_DescriptionText->SetText(FText::FromString(Cause));
}

void UP1LoginMenuWidget::RequestCreateCharacter(const FString& CharacterName, EP1CharacterClass CharacterClass)
{
    if (CharacterName.Len() <= 0)
    {
        CC_DescriptionText->SetText(FText::FromString(TEXT("캐릭터 이름을 입력해주세요.")));
        return;
    }

    if (CharacterClass == EP1CharacterClass::None)
    {
        CC_DescriptionText->SetText(FText::FromString(TEXT("직업을 선택하지 않았습니다.")));
        return;
    }

    if (UP1LobbySubsystem* Lobby = GetLobby())
        Lobby->CreateCharacter(CharacterName, CharacterClass);
}

void UP1LoginMenuWidget::OnWarriorButtonClicked()
{
    SelectClass(EP1CharacterClass::Warrior);
}

void UP1LoginMenuWidget::OnMagicianButtonClicked()
{
    SelectClass(EP1CharacterClass::Mage);
}

void UP1LoginMenuWidget::OnCreateConfirmButtonClicked()
{
    RequestCreateCharacter(CC_CharacterNameText->GetText().ToString(), SelectedClass);
}

void UP1LoginMenuWidget::OnCreateCancelButtonClicked()
{
    WidgetSwitcher->SetActiveWidget(CharacterSelectScreen);
}

void UP1LoginMenuWidget::ShowDeleteCharacterFailed()
{
    WidgetSwitcher->SetActiveWidget(CharacterSelectScreen);
    ShowDescription(FText::FromString(TEXT("서버 오류: 캐릭터 삭제 실패")));
}

void UP1LoginMenuWidget::RequestDeleteCharacter()
{
    const int64 CharacterId = GetSelectedCharacterId();
    if (CharacterId == -1)
    {
        WidgetSwitcher->SetActiveWidget(CharacterSelectScreen);
        ShowDescription(FText::FromString(TEXT("삭제할 캐릭터가 없습니다.")));
        return;
    }

    if (UP1LobbySubsystem* Lobby = GetLobby())
        Lobby->DeleteCharacter(CharacterId);
}

void UP1LoginMenuWidget::OnDeleteConfirmButtonClicked()
{
    RequestDeleteCharacter();
}

void UP1LoginMenuWidget::OnDeleteCancelButtonClicked()
{
    WidgetSwitcher->SetActiveWidget(CharacterSelectScreen);
}
