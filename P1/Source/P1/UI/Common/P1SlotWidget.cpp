#include "UI/Common/P1SlotWidget.h"
#include "UI/Common/P1ItemTooltipWidget.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "TimerManager.h"
#include "Core/P1GameInstance.h"
#include "Game/Data/P1ItemAssetData.h"
#include "Game/Inventory/P1ItemCooldown.h"
#include "Game/Progress/P1MyPlayerData.h"

void UP1SlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (TooltipClass && !SlotTooltipWidget)
        SlotTooltipWidget = CreateWidget<UP1ItemTooltipWidget>(this, TooltipClass);

    if (UP1GameInstance* GameInstance = GetP1GameInstance())
    {
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
            MyPlayerData->OnItemCooldownStarted.AddUObject(this, &UP1SlotWidget::HandleItemCooldownStarted);
    }
}

void UP1SlotWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    // 디자이너 미리보기에서도 돈다. 테이블이나 위젯이 없을 수 있다.
    if (DisplayTemplateId > 0)
    {
        if (ItemTable == nullptr)
            return;

        if (const FP1ItemData* Row = ItemTable->FindRow<FP1ItemData>(
            FName(*FString::FromInt(DisplayTemplateId)), TEXT("UP1SlotWidget::NativePreConstruct"), false))
            SetSlot(*Row, 1);
    }
    else if (ItemIcon)
    {
        ItemIcon->SetBrushFromTexture(DisplayIcon ? DisplayIcon.Get() : SlotDefaultIcon.Get());
    }
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
    // 내 플레이어 데이터의 사본에 그대로 남는다. 위젯이 다시 붙으면 NativePreConstruct가 아이콘을
    // 초기 텍스처로 되돌리고, 사본의 MODIFIED 슬롯으로 다시 그리면 아이콘이 빈 채로 남았다.
    // 한 번도 채워진 적 없는 빈 슬롯(NONE)은 건드리지 않아 초기 텍스처를 남긴다.
    if (_Slot.has_item())
        InsertData(_Slot);
    else if (_Slot.state() == Protocol::UpdateState::UPDATE_STATE_REMOVED)
        ClearSlot();
    
    if (SlotData.item().count() > 1)
        ItemCountText->SetText(FText::AsNumber(SlotData.item().count()));
    else
        ItemCountText->SetText(FText::GetEmpty());

    // 대기 중인 물약이 칸에 새로 들어왔거나 칸이 비었을 수 있다.
    RefreshCooldown();
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

int32 UP1SlotWidget::GetHeldTemplateId() const
{
    // ItemData가 아니라 SlotData로 본다. 칸을 비워도 ItemData에는 지난 아이템이 남는다(#163).
    return SlotData.has_item() ? SlotData.item().template_id() : 0;
}

void UP1SlotWidget::HandleItemCooldownStarted(int32 TemplateId)
{
    if (TemplateId == GetHeldTemplateId())
        RefreshCooldown();
}

void UP1SlotWidget::RefreshCooldown()
{
    if (CooldownBar == nullptr)
        return;

    UP1GameInstance* GameInstance = GetP1GameInstance();
    const UP1MyPlayerData* MyPlayerData = GameInstance ? GameInstance->GetSubsystem<UP1MyPlayerData>() : nullptr;
    const int32 TemplateId = GetHeldTemplateId();
    const FP1ItemCooldown* Cooldown = (MyPlayerData && TemplateId > 0) ? MyPlayerData->FindActiveItemCooldown(TemplateId) : nullptr;

    UWorld* World = GetWorld();
    if (Cooldown == nullptr || World == nullptr)
    {
        // 블루프린트는 끝날 때 막대를 0 이하의 마지막 값으로 두었다. 0으로 맞춰도 보이는 것은 같다.
        CooldownBar->SetPercent(0.f);
        if (World)
            World->GetTimerManager().ClearTimer(CooldownTimerHandle);
        return;
    }

    CooldownBar->SetPercent(Cooldown->GetRemainingRatio(FP1ItemCooldown::GetClockSeconds()));
    if (World->GetTimerManager().IsTimerActive(CooldownTimerHandle) == false)
        World->GetTimerManager().SetTimer(CooldownTimerHandle, this, &UP1SlotWidget::RefreshCooldown, CooldownBarIntervalSeconds, true);
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
