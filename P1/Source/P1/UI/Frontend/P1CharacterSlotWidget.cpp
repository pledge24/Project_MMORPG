#include "UI/Frontend/P1CharacterSlotWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Lobby/P1LobbyCharacter.h"
#include "Utils/LogCategory.h"

void UP1CharacterSlotWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    SlotButton0->OnClicked.AddDynamic(this, &UP1CharacterSlotWidget::OnSlotButtonClicked);
}

void UP1CharacterSlotWidget::SetSlotId(int32 InSlotId)
{
    SlotId = InSlotId;
}

void UP1CharacterSlotWidget::ShowCharacter(const FP1LobbyCharacter& Character)
{
    CharacterId = Character.CharacterId;
    Name_txt->SetText(FText::FromString(Character.CharacterName));
    Info_txt->SetText(FText::FromString(FString::Printf(TEXT("%s Lv.%d"), *GetClassDisplayName(Character.CharacterClass), Character.CharacterLevel)));
}

void UP1CharacterSlotWidget::Clear()
{
    CharacterId = -1;
    Info_txt->SetText(FText::GetEmpty());
    Name_txt->SetText(FText::FromString(TEXT("캐릭터 없음")));
}

void UP1CharacterSlotWidget::SetHighlighted(bool bHighlighted)
{
    Highlight_img->SetVisibility(bHighlighted ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}

bool UP1CharacterSlotWidget::HasCharacter() const
{
    return CharacterId != -1;
}

FString UP1CharacterSlotWidget::GetClassDisplayName(EP1CharacterClass CharacterClass)
{
    switch (CharacterClass)
    {
    case EP1CharacterClass::Warrior:
        return TEXT("전사");
    case EP1CharacterClass::Mage:
        return TEXT("마법사");
    default:
        break;
    }

    // 서버 데이터에 직업이 더해져도 화면이 멈추지 않도록 대체 이름으로 보여 준다.
    UE_LOG(LogP1UI, Warning, TEXT("이름이 없는 직업 값: %d"), static_cast<int32>(CharacterClass));
    return TEXT("알 수 없음");
}

void UP1CharacterSlotWidget::OnSlotButtonClicked()
{
    OnSlotClicked.ExecuteIfBound(SlotId);
}
