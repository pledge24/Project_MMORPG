#pragma once

#include "CoreMinimal.h"
#include "UI/Common/P1ContainerWidget.h"
#include "P1SlotGridWidget.generated.h"

class UUniformGridPanel;

/**
 * 컨테이너의 칸을 격자로 그리는 위젯.
 *
 * 칸 번호 순서대로 슬롯 위젯을 왼쪽에서 오른쪽, 위에서 아래로 늘어놓는다. 인벤토리의 구역이 쓴다.
 */
UCLASS()
class P1_API UP1SlotGridWidget : public UP1ContainerWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativePreConstruct() override;
    //~ End UUserWidget Interface

    //~ Begin UP1ContainerWidget Interface
protected:
    virtual void CollectSlotWidgets(TMap<int32, TObjectPtr<UP1SlotWidget>>& OutSlotWidgets) override;
    //~ End UP1ContainerWidget Interface

    //~ Grid
private:
    /** 격자의 슬롯 위젯이 SlotCount개가 아니면 모두 지우고 다시 만든다. */
    void BuildSlots();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> SlotGrid;

    UPROPERTY(EditAnywhere, Category = "Grid")
    TSubclassOf<UP1SlotWidget> SlotWidgetClass;

    /** 칸 수. i번째 슬롯 위젯이 i번 칸을 그린다. */
    UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = 0))
    int32 SlotCount = 0;

    UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = 1))
    int32 Columns = 1;
};
