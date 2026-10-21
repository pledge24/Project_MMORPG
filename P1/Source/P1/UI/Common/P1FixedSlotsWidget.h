#pragma once

#include "CoreMinimal.h"
#include "UI/Common/P1ContainerWidget.h"
#include "P1FixedSlotsWidget.generated.h"

/**
 * 컨테이너의 칸을 디자이너가 놓은 자리에 그리는 위젯.
 *
 * 칸마다 정해진 자리가 있는 컨테이너에 쓴다. 착용 장비가 쓴다.
 */
UCLASS()
class P1_API UP1FixedSlotsWidget : public UP1ContainerWidget
{
    GENERATED_BODY()

    //~ Begin UP1ContainerWidget Interface
protected:
    virtual void CollectSlotWidgets(TMap<int32, TObjectPtr<UP1SlotWidget>>& OutSlotWidgets) override;
    //~ End UP1ContainerWidget Interface

    //~ Placement
private:
    /** 이 위젯 안의 슬롯 위젯 이름과 그 위젯이 그릴 칸 번호. */
    UPROPERTY(EditAnywhere, Category = "Container")
    TMap<FName, int32> SlotIndexByWidgetName;
};
