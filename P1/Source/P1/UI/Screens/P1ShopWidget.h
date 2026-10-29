#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1ShopWidget.generated.h"

class UButton;
class UP1SlotWidget;
class UPanelWidget;

/**
 * 상점 창.
 *
 * 상점 표의 판매 목록을 진열하고, 진열 칸의 우클릭을 구매 의도로 바꿔 아이템 요청 창구에 넘긴다.
 */
UCLASS()
class P1_API UP1ShopWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
public:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;

protected:
    /** 상점이 열린 동안은 이 창만 키를 받는다. 메뉴 키는 상점을 닫고, 채팅 키는 채팅 입력칸을 연다. */
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    //~ End UUserWidget Interface

    //~ Display
public:
    /** ShopId 상점의 판매 목록을 진열 칸에 앞에서부터 채운다. 남는 칸은 비운다. 상점 표에 없으면 모두 비운다. */
    void SetShop(int32 ShopId);

    //~ Close
private:
    UFUNCTION()
    void HandleCloseClicked();

    /** 상점을 닫고 카메라를 되돌린다. 컨트롤러가 없으면 아무것도 하지 않는다. */
    void CloseShop();

    /** 상점을 닫는 버튼. WBP에 없으면 메뉴 키로만 닫는다. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Close_btn;

    //~ Purchase
private:
    /** 우클릭한 진열 칸의 아이템 하나를 산다. */
    void HandleSlotRightClicked(UP1SlotWidget* SlotWidget);

    /** 진열 칸을 담은 패널. 자식 가운데 슬롯 위젯만 구매를 받는다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> UGP_Shop;
};
