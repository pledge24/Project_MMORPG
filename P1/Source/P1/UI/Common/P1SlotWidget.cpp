#include "UI/Common/P1SlotWidget.h"
#include "UI/Common/P1ItemTooltipWidget.h"
#include "UI/Common/P1SlotDragDropOperation.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "Core/P1GameInstance.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/P1ItemData.h"
#include "Game/Inventory/P1ItemCooldown.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Utils/LogCategory.h"

void UP1SlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 툴팁 위젯은 칸마다 한 번만 만들고 띄울 때마다 다시 채운다.
    if (TooltipClass && !SlotTooltipWidget)
        SlotTooltipWidget = CreateWidget<UP1ItemTooltipWidget>(this, TooltipClass);

    // 재사용 대기 시작 알림 구독. 떼는 것은 UP1UserWidget::NativeDestruct가 한다.
    if (UP1GameInstance* GameInstance = GetP1GameInstance())
    {
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
            MyPlayerData->OnItemCooldownStarted.AddUObject(this, &UP1SlotWidget::HandleItemCooldownStarted);
    }
}

void UP1SlotWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 툴팁은 띄울 때마다 지금 그리는 아이템으로 채운다.
    // 슬레이트 위젯을 만들 때 델리게이트가 묶여 있어야 툴팁이 걸리므로 NativeConstruct보다 앞인 여기서 묶는다.
    if (ItemIcon)
        ItemIcon->ToolTipWidgetDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UP1SlotWidget, GetToolTipWidget));
}

void UP1SlotWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    // 디자이너 미리보기에서도 돈다. 진열 칸은 아이템 하나를 대기 없이 그리고, 나머지는 빈 칸으로 시작한다.
    // 칸을 그리는 위젯은 자식인 이 위젯보다 늦게 Construct되므로 이 초기값을 덮어쓴다.
    if (DisplayTemplateId > 0)
        SetDisplay(FP1SlotDisplay{ DisplayTemplateId, 1, 0 });
    else
        SetDisplay(FP1SlotDisplay());
}

FReply UP1SlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && IsOverIcon(InMouseEvent))
    {
        OnRightClicked.Broadcast(this);
        return FReply::Handled();
    }

    // 끌 수 있는 칸만 끌기를 감지한다. 감지하면 마우스를 잡으므로, 끌 수 없는 칸에서 잡으면 바깥 창이 눌림을 받지 못한다.
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && OnDragRequested.IsBound() && Display.IsEmpty() == false
        && IsOverIcon(InMouseEvent))
    {
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
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

void UP1SlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

    const TOptional<FP1SlotDragPayload> Payload = OnDragRequested.IsBound() ? OnDragRequested.Execute(this) : TOptional<FP1SlotDragPayload>();
    if (Payload.IsSet() == false)
        return;

    UP1SlotDragDropOperation* Operation = NewObject<UP1SlotDragDropOperation>(this);
    Operation->Payload = Payload.GetValue();
    Operation->Pivot = EDragPivot::CenterCenter;

    // 끌리는 그림은 같은 슬롯 위젯으로 그린다. 끄는 동안 놓을 자리를 가리지 않게 입력을 받지 않는다.
    if (UP1SlotWidget* DragVisual = CreateWidget<UP1SlotWidget>(this, GetClass()))
    {
        DragVisual->SetVisibility(ESlateVisibility::HitTestInvisible);
        DragVisual->SetDisplay(Payload->Display);
        Operation->DefaultDragVisual = DragVisual;
    }

    OutOperation = Operation;
}

bool UP1SlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    // 받아들이지 않으면 바깥에 놓은 것이 되어 끌기를 시작한 슬롯이 취소를 알린다.
    const UP1SlotDragDropOperation* Operation = Cast<UP1SlotDragDropOperation>(InOperation);
    if (Operation && OnDropped.IsBound() && OnDropped.Execute(this, Operation->Payload))
        return true;

    return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UP1SlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

    if (const UP1SlotDragDropOperation* Operation = Cast<UP1SlotDragDropOperation>(InOperation))
        OnDragCancelled.Broadcast(this, Operation->Payload);
}

bool UP1SlotWidget::IsOverIcon(const FPointerEvent& InMouseEvent) const
{
    // 슬롯 테두리가 아니라 아이콘 영역만 받는다.
    return ItemIcon && ItemIcon->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition());
}

void UP1SlotWidget::SetDisplay(const FP1SlotDisplay& InDisplay)
{
    Display = InDisplay;

    if (ItemIcon)
    {
        UTexture2D* Icon = Display.IsEmpty() ? nullptr : UP1GameDataSettings::LoadItemIcon(Display.TemplateId);
        if (Display.IsEmpty() == false && Icon == nullptr)
            UE_LOG(LogP1UI, Warning, TEXT("아이콘이 없는 템플릿 id: %d"), Display.TemplateId);

        // 아이콘을 찾지 못한 아이템도 빈 칸 그림으로 보인다. 앞서 그린 아이템의 아이콘이 남으면 다른 아이템으로 보인다.
        if (Icon == nullptr)
            Icon = DisplayIcon ? DisplayIcon.Get() : SlotDefaultIcon.Get();
        ItemIcon->SetBrushFromTexture(Icon);
        if (NormalIconColor.IsSet() == false)
            NormalIconColor = ItemIcon->GetColorAndOpacity();
        ItemIcon->SetColorAndOpacity(Display.bDimmed ? DimmedIconColor : NormalIconColor.GetValue());
    }

    if (ItemCountText)
    {
        const bool bShowCount = Display.Count > 1 || (Display.bAlwaysShowCount && Display.IsEmpty() == false);
        ItemCountText->SetText(bShowCount ? FText::AsNumber(Display.Count) : FText::GetEmpty());
    }

    // 대기 중인 물약이 칸에 새로 들어왔거나 칸이 비었을 수 있다.
    RefreshCooldown();
}

void UP1SlotWidget::HandleItemCooldownStarted(int32 TemplateId)
{
    if (TemplateId == Display.CooldownTemplateId)
        RefreshCooldown();
}

void UP1SlotWidget::RefreshCooldown()
{
    if (CooldownBar == nullptr)
        return;

    UP1GameInstance* GameInstance = GetP1GameInstance();
    const UP1MyPlayerData* MyPlayerData = GameInstance ? GameInstance->GetSubsystem<UP1MyPlayerData>() : nullptr;
    const int32 TemplateId = Display.CooldownTemplateId;
    const FP1ItemCooldown* Cooldown = (MyPlayerData && TemplateId > 0) ? MyPlayerData->FindActiveItemCooldown(TemplateId) : nullptr;

    UWorld* World = GetWorld();
    if (Cooldown == nullptr || World == nullptr)
    {
        CooldownBar->SetPercent(0.f);
        if (World)
            World->GetTimerManager().ClearTimer(CooldownTimerHandle);
        return;
    }

    CooldownBar->SetPercent(Cooldown->GetRemainingRatio(FP1ItemCooldown::GetClockSeconds()));
    if (World->GetTimerManager().IsTimerActive(CooldownTimerHandle) == false)
        World->GetTimerManager().SetTimer(CooldownTimerHandle, this, &UP1SlotWidget::RefreshCooldown, CooldownBarIntervalSeconds, true);
}

UWidget* UP1SlotWidget::GetToolTipWidget() const
{
    if (Display.IsEmpty() || SlotTooltipWidget == nullptr)
        return nullptr;

    const FP1ItemData* ItemData = UP1GameDataSettings::FindItemData(Display.TemplateId);
    if (ItemData == nullptr)
        return nullptr;

    SlotTooltipWidget->Init(*ItemData, UP1GameDataSettings::LoadItemIcon(Display.TemplateId));
    return SlotTooltipWidget;
}
