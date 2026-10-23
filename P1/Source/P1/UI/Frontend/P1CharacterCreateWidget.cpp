#include "UI/Frontend/P1CharacterCreateWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Lobby/P1LobbySubsystem.h"
#include "UI/Frontend/P1CharacterSlotWidget.h"

void UP1CharacterCreateWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    WarriorButton->OnClicked.AddDynamic(this, &UP1CharacterCreateWidget::OnWarriorButtonClicked);
    MageButton->OnClicked.AddDynamic(this, &UP1CharacterCreateWidget::OnMageButtonClicked);
    CreateButton->OnClicked.AddDynamic(this, &UP1CharacterCreateWidget::OnCreateButtonClicked);
    CancelButton->OnClicked.AddDynamic(this, &UP1CharacterCreateWidget::OnCancelButtonClicked);
}

void UP1CharacterCreateWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 응답 구독. 떼는 것은 NativeDestruct가 한다.
    if (UP1LobbySubsystem* Lobby = GetLobby())
        Lobby->OnCreateCharacterFailed.AddUObject(this, &UP1CharacterCreateWidget::ShowCreateFailed);
}

void UP1CharacterCreateWidget::NativeDestruct()
{
    // 서브시스템은 레벨보다 오래 산다. 떼지 않으면 로그인 맵을 다시 열 때마다 죽은 위젯의 항목이 쌓인다.
    if (UP1LobbySubsystem* Lobby = GetLobby())
        Lobby->RemoveListener(this);

    Super::NativeDestruct();
}

void UP1CharacterCreateWidget::Reset()
{
    SelectedClass = EP1CharacterClass::None;
    SelectedClassText->SetText(FText::FromString(TEXT("직업을 고르세요")));
    DescriptionText->SetText(FText::GetEmpty());
    NameBox->SetText(FText::GetEmpty());

    // 직업 목록은 서버의 로비 표가 정한다. 클라이언트에 따로 두면 로비 표를 바꿀 때 클라이언트도 패치해야 한다.
    const UP1LobbySubsystem* Lobby = GetLobby();
    const TArray<EP1CharacterClass> CreatableClasses = Lobby ? Lobby->GetCreatableClasses() : TArray<EP1CharacterClass>();

    const bool bWarriorCreatable = CreatableClasses.Contains(EP1CharacterClass::Warrior);
    const bool bMageCreatable = CreatableClasses.Contains(EP1CharacterClass::Mage);
    WarriorButton->SetVisibility(bWarriorCreatable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    MageButton->SetVisibility(bMageCreatable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    if (CreatableClasses.Num() == 1)
        SelectClass(CreatableClasses[0]);
}

UP1LobbySubsystem* UP1CharacterCreateWidget::GetLobby() const
{
    UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UP1LobbySubsystem>() : nullptr;
}

void UP1CharacterCreateWidget::SelectClass(EP1CharacterClass CharacterClass)
{
    SelectedClass = CharacterClass;
    SelectedClassText->SetText(FText::FromString(UP1CharacterSlotWidget::GetClassDisplayName(CharacterClass)));
}

void UP1CharacterCreateWidget::ShowCreateFailed(const FText& Message)
{
    DescriptionText->SetText(Message);
}

void UP1CharacterCreateWidget::OnWarriorButtonClicked()
{
    SelectClass(EP1CharacterClass::Warrior);
}

void UP1CharacterCreateWidget::OnMageButtonClicked()
{
    SelectClass(EP1CharacterClass::Mage);
}

void UP1CharacterCreateWidget::OnCreateButtonClicked()
{
    // 이름의 길이와 직업은 서버가 다시 검사한다. 여기서는 비어 있는 요청만 보내지 않는다.
    const FString CharacterName = NameBox->GetText().ToString();
    if (CharacterName.IsEmpty())
    {
        DescriptionText->SetText(FText::FromString(TEXT("캐릭터 이름을 입력해 주세요.")));
        return;
    }

    if (SelectedClass == EP1CharacterClass::None)
    {
        DescriptionText->SetText(FText::FromString(TEXT("직업을 선택하지 않았습니다.")));
        return;
    }

    if (UP1LobbySubsystem* Lobby = GetLobby())
        Lobby->CreateCharacter(CharacterName, SelectedClass);
}

void UP1CharacterCreateWidget::OnCancelButtonClicked()
{
    OnClosed.Broadcast();
}
