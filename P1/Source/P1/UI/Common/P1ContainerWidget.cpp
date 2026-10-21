#include "UI/Common/P1ContainerWidget.h"
#include "UI/Common/P1SlotWidget.h"
#include "Core/P1GameInstance.h"
#include "Game/Items/P1ItemContainer.h"
#include "Game/Progress/P1ItemActions.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Utils/LogCategory.h"

void UP1ContainerWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 슬롯 위젯 대응과 클릭 연결. 위젯이 사는 동안 한 번만 한다.
    CollectSlotWidgets(SlotWidgets);
    for (const auto& Pair : SlotWidgets)
    {
        if (Pair.Value == nullptr)
            continue;

        Pair.Value->OnDoubleClicked.AddUObject(this, &UP1ContainerWidget::HandleSlotDoubleClicked);
        Pair.Value->OnRightClicked.AddUObject(this, &UP1ContainerWidget::HandleSlotRightClicked);
        if (bSlotsDraggable)
            Pair.Value->OnDragRequested.BindUObject(this, &UP1ContainerWidget::MakeDragPayload);
    }
}

void UP1ContainerWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 지금 칸으로 그리고 이후 바뀌는 칸을 구독. 떼는 것은 UP1UserWidget::NativeDestruct가 한다.
    UP1GameInstance* GameInstance = GetP1GameInstance();
    UP1MyPlayerData* MyPlayerData = GameInstance ? GameInstance->GetSubsystem<UP1MyPlayerData>() : nullptr;
    UP1ItemContainer* Container = MyPlayerData ? MyPlayerData->GetContainer(ContainerType) : nullptr;
    if (Container == nullptr)
    {
        UE_LOG(LogP1UI, Warning, TEXT("%s: 그릴 컨테이너가 없다. 컨테이너 %d"), *GetName(), static_cast<int32>(ContainerType));
        return;
    }

    Redraw(*Container);
    Container->OnSlotChanged.AddUObject(this, &UP1ContainerWidget::HandleSlotChanged);
    Container->OnReset.AddUObject(this, &UP1ContainerWidget::Redraw);
}

void UP1ContainerWidget::HandleSlotChanged(const FP1Slot& Changed)
{
    if (const TObjectPtr<UP1SlotWidget>* SlotWidget = SlotWidgets.Find(Changed.Address.SlotIndex))
    {
        if (*SlotWidget)
            (*SlotWidget)->SetDisplay(FP1SlotDisplay::FromSlot(Changed));
    }
}

void UP1ContainerWidget::Redraw(const UP1ItemContainer& Container)
{
    // 컨테이너에 없는 칸 번호의 위젯은 빈 칸으로 그린다. 그러지 않으면 접속이 끊긴 뒤 이전 캐릭터의 칸이 남는다.
    for (const auto& Pair : SlotWidgets)
    {
        if (Pair.Value == nullptr)
            continue;

        const FP1Slot* Found = Container.FindSlot(Pair.Key);
        Pair.Value->SetDisplay(Found ? FP1SlotDisplay::FromSlot(*Found) : FP1SlotDisplay());
    }
}

void UP1ContainerWidget::HandleSlotDoubleClicked(UP1SlotWidget* SlotWidget)
{
    RequestIntent(DoubleClickIntent, SlotWidget);
}

void UP1ContainerWidget::HandleSlotRightClicked(UP1SlotWidget* SlotWidget)
{
    RequestIntent(RightClickIntent, SlotWidget);
}

void UP1ContainerWidget::RequestIntent(EP1ItemIntent Intent, UP1SlotWidget* SlotWidget) const
{
    if (Intent == EP1ItemIntent::None)
        return;

    const int32* SlotIndex = SlotWidgets.FindKey(SlotWidget);
    UP1GameInstance* GameInstance = GetP1GameInstance();
    UP1MyPlayerData* MyPlayerData = GameInstance ? GameInstance->GetSubsystem<UP1MyPlayerData>() : nullptr;
    if (SlotIndex == nullptr || MyPlayerData == nullptr)
        return;

    FP1SlotAddress Address;
    Address.Container = ContainerType;
    Address.SlotIndex = *SlotIndex;
    MyPlayerData->GetItemActions()->RequestForSlot(Intent, Address);
}

TOptional<FP1SlotDragPayload> UP1ContainerWidget::MakeDragPayload(const UP1SlotWidget* SlotWidget) const
{
    if (SlotWidget == nullptr)
        return {};

    const int32* SlotIndex = SlotWidgets.FindKey(const_cast<UP1SlotWidget*>(SlotWidget));
    if (SlotIndex == nullptr)
        return {};

    FP1SlotDragPayload Payload;
    Payload.Origin = FP1SlotDragPayload::EOrigin::ContainerSlot;
    Payload.Address.Container = ContainerType;
    Payload.Address.SlotIndex = *SlotIndex;
    Payload.Display = SlotWidget->GetDisplay();
    return Payload;
}
