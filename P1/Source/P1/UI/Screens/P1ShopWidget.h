#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1ShopWidget.generated.h"

class UP1SlotWidget;
class UPanelWidget;

/**
 * 상점 창.
 *
 * 상점이 파는 아이템을 진열하고, 진열 칸의 우클릭을 구매 의도로 바꿔 아이템 요청 창구에 넘긴다.
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
private:
    /** 우클릭한 진열 칸의 아이템 하나를 산다. */
    void HandleSlotRightClicked(UP1SlotWidget* SlotWidget);

    /** 진열 칸을 담은 패널. 자식 가운데 슬롯 위젯만 구매를 받는다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> UGP_Shop;
};
