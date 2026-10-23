#include "UI/Frontend/P1CharacterDeleteWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Lobby/P1LobbyCharacter.h"
#include "Lobby/P1LobbySubsystem.h"
#include "UI/Frontend/P1CharacterSlotWidget.h"

void UP1CharacterDeleteWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    NameBox->OnTextChanged.AddDynamic(this, &UP1CharacterDeleteWidget::OnNameChanged);
    ConfirmButton->OnClicked.AddDynamic(this, &UP1CharacterDeleteWidget::OnConfirmButtonClicked);
    CancelButton->OnClicked.AddDynamic(this, &UP1CharacterDeleteWidget::OnCancelButtonClicked);
}

void UP1CharacterDeleteWidget::SetCharacter(const FP1LobbyCharacter& Character)
{
    CharacterId = Character.CharacterId;
    CharacterName = Character.CharacterName;

    CharacterInfoText->SetText(FText::FromString(FString::Printf(TEXT("%s (%s · Lv. %d)을 삭제합니다.\n삭제한 캐릭터는 되돌릴 수 없습니다."),
        *Character.CharacterName, *UP1CharacterSlotWidget::GetClassDisplayName(Character.CharacterClass), Character.CharacterLevel)));
    NameBox->SetText(FText::GetEmpty());
    ConfirmButton->SetIsEnabled(false);
}

void UP1CharacterDeleteWidget::OnNameChanged(const FText& Text)
{
    // 되돌릴 수 없는 삭제라서 고른 캐릭터를 한 번 더 확인하게 한다. 대소문자도 같아야 한다.
    ConfirmButton->SetIsEnabled(CharacterId != -1 && Text.ToString().Equals(CharacterName, ESearchCase::CaseSensitive));
}

void UP1CharacterDeleteWidget::OnConfirmButtonClicked()
{
    UGameInstance* GameInstance = GetGameInstance();
    if (UP1LobbySubsystem* Lobby = GameInstance ? GameInstance->GetSubsystem<UP1LobbySubsystem>() : nullptr)
        Lobby->DeleteCharacter(CharacterId);

    // 응답이 오기 전에 다시 누르지 못하게 한다. 성공과 실패 모두 로그인 메뉴가 이 창을 닫는다.
    ConfirmButton->SetIsEnabled(false);
}

void UP1CharacterDeleteWidget::OnCancelButtonClicked()
{
    OnClosed.Broadcast();
}
