#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "UI/Common/P1SlotDragPayload.h"
#include "P1QuickSlotsWidget.generated.h"

class UP1ItemContainer;
class UP1MyPlayerData;
class UP1QuickSlots;
class UP1SlotWidget;
struct FP1QuickSlot;
struct FP1Slot;

/**
 * 내 플레이어의 퀵슬롯을 디자이너가 놓은 자리에 그리는 위젯.
 *
 * 퀵슬롯마다 슬롯 위젯 하나로 그 소모품의 소모품 구역 전체 개수를 보여 준다.
 * 소모품 칸이나 다른 퀵슬롯을 놓으면 등록과 교환을, 퀵슬롯을 바깥에 놓으면 해제를 요청한다.
 */
UCLASS()
class P1_API UP1QuickSlotsWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Slot Widgets
private:
    /** 이 위젯 안의 슬롯 위젯을 트리 순서대로 든다. i번째 슬롯 위젯이 i번 퀵슬롯을 그린다. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UP1SlotWidget>> SlotWidgets;

    //~ Drawing
private:
    void HandleQuickSlotsReset(const UP1QuickSlots& QuickSlots);
    void HandleQuickSlotChanged(const FP1QuickSlot& Changed);
    void HandleConsumableSlotChanged(const FP1Slot& Changed);
    void HandleConsumablesReset(const UP1ItemContainer& Container);

    /** 슬롯 위젯을 모두 지금 퀵슬롯과 소모품 구역으로 다시 그린다. 서버가 알려 주지 않은 번호의 슬롯은 빈 칸이다. */
    void Redraw();

    //~ Drag and Drop
private:
    TOptional<FP1SlotDragPayload> MakeDragPayload(const UP1SlotWidget* SlotWidget) const;

    /** 소모품 칸이면 등록을, 다른 퀵슬롯이면 교환을 요청한다. 퀵슬롯 위에 놓은 끌기는 요청이 거절돼도 받아들인다. */
    bool HandleDrop(UP1SlotWidget* Target, const FP1SlotDragPayload& Payload);

    /** 퀵슬롯을 받아들인 곳 없이 놓았으면 해제를 요청한다. */
    void HandleDragCancelled(UP1SlotWidget* Source, const FP1SlotDragPayload& Payload);

    //~ 공통
private:
    /** 이 위젯의 슬롯 위젯이 그리는 퀵슬롯 번호. 이 위젯의 슬롯이 아니거나 서버가 알려 주지 않은 번호면 INDEX_NONE. */
    int32 FindQuickSlotIndex(const UP1SlotWidget* SlotWidget) const;

    UP1MyPlayerData* GetMyPlayerData() const;
};
