#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1ShopWidget.generated.h"

class UP1SlotWidget;
class UPanelWidget;

/**
 * 상점 창.
 *
 * 상점이 파는 아이템을 진열하고, 진열 칸 조작을 구매 요청으로 바꾼다.
 */
UCLASS()
class P1_API UP1ShopWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
public:
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Purchase
protected:
    /** 구매 응답을 기다리는 동안 true. 중복 요청을 막는다. */
    UPROPERTY(VisibleAnywhere)
    bool bPendingPacket = false;

private:
    /** 우클릭한 진열 칸의 아이템을 하나 산다. 골드가 모자라면 요청하지 않는다. */
    void HandleSlotRightClicked(UP1SlotWidget* SlotWidget);

    /** 진열 칸을 담은 패널. 자식 가운데 슬롯 위젯만 구매를 받는다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> UGP_Shop;
};
