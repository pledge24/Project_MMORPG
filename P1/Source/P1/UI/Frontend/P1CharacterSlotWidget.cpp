#include "UI/Frontend/P1CharacterSlotWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Lobby/P1LobbyCharacter.h"
#include "Utils/LogCategory.h"

void UP1CharacterSlotWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    SlotButton->OnClicked.AddDynamic(this, &UP1CharacterSlotWidget::OnSlotButtonClicked);
    ApplyState();
}

void UP1CharacterSlotWidget::SetSlotId(int32 InSlotId)
{
    SlotId = InSlotId;
}

void UP1CharacterSlotWidget::ShowCharacter(const FP1LobbyCharacter& Character)
{
    CharacterId = Character.CharacterId;
    CharacterClass = Character.CharacterClass;
    NameText->SetText(FText::FromString(Character.CharacterName));
    InfoText->SetText(FText::FromString(FString::Printf(TEXT("%s · Lv. %d"), *GetClassDisplayName(Character.CharacterClass), Character.CharacterLevel)));
    ApplyState();
}

void UP1CharacterSlotWidget::Clear()
{
    CharacterId = -1;
    CharacterClass = EP1CharacterClass::None;
    NameText->SetText(FText::GetEmpty());
    InfoText->SetText(FText::GetEmpty());
    ApplyState();
}

void UP1CharacterSlotWidget::SetHighlighted(bool bInHighlighted)
{
    bHighlighted = bInHighlighted;
    ApplyState();
}

bool UP1CharacterSlotWidget::HasCharacter() const
{
    return CharacterId != -1;
}

FString UP1CharacterSlotWidget::GetClassDisplayName(EP1CharacterClass InCharacterClass)
{
    switch (InCharacterClass)
    {
    case EP1CharacterClass::Warrior:
        return TEXT("전사");
    case EP1CharacterClass::Mage:
        return TEXT("마법사");
    default:
        break;
    }

    // 서버 데이터에 직업이 더해져도 화면이 멈추지 않도록 대체 이름으로 보여 준다.
    UE_LOG(LogP1UI, Warning, TEXT("이름이 없는 직업 값: %d"), static_cast<int32>(InCharacterClass));
    return TEXT("알 수 없음");
}

void UP1CharacterSlotWidget::ApplyState()
{
    const bool bHasCharacter = HasCharacter();
    const auto ShowIf = [](UWidget* Widget, bool bShow)
    {
        Widget->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    };

    // 고른 슬롯은 캐릭터 유무와 상관없이 고른 배경을 쓴다.
    ShowIf(SelectedBackground, bHighlighted);
    ShowIf(FilledBackground, bHighlighted == false && bHasCharacter);
    ShowIf(EmptyBackground, bHighlighted == false && bHasCharacter == false);

    ShowIf(CharacterContent, bHasCharacter);
    ShowIf(EmptyContent, bHasCharacter == false);

    // 문장이 없는 직업은 문장 없이 이름만 보인다.
    ShowIf(WarriorEmblem, CharacterClass == EP1CharacterClass::Warrior);
    ShowIf(MageEmblem, CharacterClass == EP1CharacterClass::Mage);
}

void UP1CharacterSlotWidget::OnSlotButtonClicked()
{
    OnSlotClicked.ExecuteIfBound(SlotId);
}
