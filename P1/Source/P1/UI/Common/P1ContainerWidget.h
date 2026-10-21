#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/Items/P1ContainerType.h"
#include "Game/Items/P1Slot.h"
#include "Game/Progress/P1ItemIntent.h"
#include "P1ContainerWidget.generated.h"

class UP1ItemContainer;
class UP1SlotWidget;

/**
 * 내 플레이어의 컨테이너 하나를 그리는 위젯의 베이스.
 *
 * 칸 번호와 슬롯 위젯의 대응을 들고 컨테이너의 칸을 그리며, 슬롯의 클릭을 그 칸 주소에 대한 의도로 바꿔 아이템 요청 창구에 넘긴다.
 * 슬롯 위젯을 어떻게 배치하는지만 하위 클래스가 정한다. 컨테이너 종류마다 하위 클래스를 두지 않는다.
 */
UCLASS(Abstract)
class P1_API UP1ContainerWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Slot Widgets
protected:
    /** 칸 번호마다 그 칸을 그릴 슬롯 위젯을 OutSlotWidgets에 넣는다. 위젯이 초기화될 때 한 번 불린다. */
    virtual void CollectSlotWidgets(TMap<int32, TObjectPtr<UP1SlotWidget>>& OutSlotWidgets) PURE_VIRTUAL(UP1ContainerWidget::CollectSlotWidgets, );

    //~ Drawing
private:
    void HandleSlotChanged(const FP1Slot& Changed);

    /** 슬롯 위젯을 모두 컨테이너의 지금 칸으로 다시 그린다. */
    void Redraw(const UP1ItemContainer& Container);

    //~ Intents
protected:
    /** 칸을 더블클릭했을 때의 의도. */
    UPROPERTY(EditAnywhere, Category = "Container")
    EP1ItemIntent DoubleClickIntent = EP1ItemIntent::None;

    /** 칸을 우클릭했을 때의 의도. */
    UPROPERTY(EditAnywhere, Category = "Container")
    EP1ItemIntent RightClickIntent = EP1ItemIntent::None;

private:
    void HandleSlotDoubleClicked(UP1SlotWidget* SlotWidget);
    void HandleSlotRightClicked(UP1SlotWidget* SlotWidget);

    /** 의도가 None이거나 이 위젯의 슬롯이 아니면 아무것도 하지 않는다. */
    void RequestIntent(EP1ItemIntent Intent, UP1SlotWidget* SlotWidget) const;

    //~ 공통
protected:
    /** 그릴 컨테이너. */
    UPROPERTY(EditAnywhere, Category = "Container")
    EP1ContainerType ContainerType = EP1ContainerType::None;

private:
    /** 칸 번호와 그 칸을 그리는 슬롯 위젯. */
    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UP1SlotWidget>> SlotWidgets;
};
