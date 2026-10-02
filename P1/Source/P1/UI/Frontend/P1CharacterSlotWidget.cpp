#include "UI/Frontend/P1CharacterSlotWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Online/P1CharacterOverview.h"

void UP1CharacterSlotWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    SlotButton0->OnClicked.AddDynamic(this, &UP1CharacterSlotWidget::OnSlotButtonClicked);
}

void UP1CharacterSlotWidget::SetSlotId(int32 InSlotId)
{
    SlotId = InSlotId;
}

void UP1CharacterSlotWidget::ShowCharacter(const FP1CharacterOverview& Character)
{
    CharacterId = Character.CharacterId;
    Name_txt->SetText(FText::FromString(Character.CharacterName));
    Info_txt->SetText(FText::FromString(FString::Printf(TEXT("%s Lv.%d"), *Character.CharacterClass, Character.CharacterLevel)));
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

void UP1CharacterSlotWidget::OnSlotButtonClicked()
{
    OnSlotClicked.ExecuteIfBound(SlotId);
}
