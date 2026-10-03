#include "UI/Common/P1SlotWidget.h"
#include "UI/Common/P1ItemTooltipWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Game/Data/P1ItemAssetData.h"

void UP1SlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (TooltipClass && !SlotTooltipWidget)
        SlotTooltipWidget = CreateWidget<UP1ItemTooltipWidget>(this, TooltipClass);
}

FReply UP1SlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && IsOverIcon(InMouseEvent))
    {
        OnRightClicked.Broadcast(this);
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UP1SlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && IsOverIcon(InMouseEvent))
    {
        OnDoubleClicked.Broadcast(this);
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

bool UP1SlotWidget::IsOverIcon(const FPointerEvent& InMouseEvent) const
{
    // 슬롯 테두리가 아니라 아이콘 영역만 받는다. 블루프린트에서 옮긴 조건이다.
    return ItemIcon && ItemIcon->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition());
}

void UP1SlotWidget::SetSlot(const FP1ItemData& Item, int32 Count)
{
    ItemData = Item;
    SlotData.mutable_item()->set_count(Count);

    ApplyIcon(Item.TemplateId);

    if (Count > 1)
        ItemCountText->SetText(FText::AsNumber(Count));
    else
        ItemCountText->SetText(FText::GetEmpty());
}

void UP1SlotWidget::SetSlot(const Protocol::Slot& _Slot)
{
    // 아이템이 있으면 state와 무관하게 데이터와 아이콘을 입힌다. state는 서버가 보낸 변경분의 표시인데
    // 내 플레이어 데이터의 사본에 그대로 남는다. 위젯이 다시 붙으면 WBP의 PreConstruct가 아이콘을
    // 초기 텍스처로 되돌리고, 사본의 MODIFIED 슬롯으로 다시 그리면 아이콘이 빈 채로 남았다.
    // 한 번도 채워진 적 없는 빈 슬롯(NONE)은 건드리지 않아 WBP의 초기 텍스처를 남긴다.
    if (_Slot.has_item())
        InsertData(_Slot);
    else if (_Slot.state() == Protocol::UpdateState::UPDATE_STATE_REMOVED)
        ClearSlot();
    
    if (SlotData.item().count() > 1)
        ItemCountText->SetText(FText::AsNumber(SlotData.item().count()));
    else
        ItemCountText->SetText(FText::GetEmpty());
}

void UP1SlotWidget::ClearSlot()
{
    if (ItemIcon) ItemIcon->SetBrushFromTexture(SlotDefaultIcon);
    if (ItemCountText) ItemCountText->SetText(FText::GetEmpty());
    SlotData.Clear();
}

void UP1SlotWidget::InsertData(const Protocol::Slot& _Slot)
{
    SlotData.CopyFrom(_Slot);
    FString TemplateId_Str = FString::FromInt(_Slot.item().template_id());

    if (ItemTable)
    {
        ItemData = *ItemTable->FindRow<FP1ItemData>(FName(*TemplateId_Str), FString("UP1SlotWidget::InsertData"));
    }

    ApplyIcon(_Slot.item().template_id());
}

void UP1SlotWidget::ApplyIcon(int32 TemplateId)
{
    if (ItemIcon == nullptr)
        return;

    if (UTexture2D* LoadedIcon = LoadIcon(TemplateId))
        ItemIcon->SetBrushFromTexture(LoadedIcon);
}

UTexture2D* UP1SlotWidget::LoadIcon(int32 TemplateId) const
{
    if (ItemAssetTable == nullptr)
        return nullptr;

    const FP1ItemAssetData* AssetData = ItemAssetTable->FindRow<FP1ItemAssetData>(
        FName(*FString::FromInt(TemplateId)), TEXT("UP1SlotWidget::LoadIcon"));
    if (AssetData == nullptr || AssetData->Icon.IsNull())
        return nullptr;

    return AssetData->Icon.LoadSynchronous();
}

UWidget* UP1SlotWidget::GetToolTipWidget_Implementation() const
{
    if (ItemData.TemplateId > 0 && TooltipClass)
    {
        if (SlotTooltipWidget)
        {
            SlotTooltipWidget->Init(ItemData, LoadIcon(ItemData.TemplateId)); // 아이템 정보 전달
            return SlotTooltipWidget;
        }
    }
    
    return nullptr;
}
