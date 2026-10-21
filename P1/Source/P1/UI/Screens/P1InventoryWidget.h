#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1InventoryWidget.generated.h"

class UButton;
class UTextBlock;
class UWidgetSwitcher;

/**
 * 내 인벤토리 창.
 *
 * 가진 아이템을 구역별 탭으로 보여 주고 골드를 보여 준다. 구역마다 격자 하나가 그 구역의 컨테이너를 그린다.
 */
UCLASS()
class P1_API UP1InventoryWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
public:
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Tabs
private:
    UFUNCTION()
    void ShowGearTab();

    UFUNCTION()
    void ShowConsumableTab();

    UFUNCTION()
    void ShowMiscTab();

    /** 0은 장비, 1은 소모품, 2는 기타 구역의 격자. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> InventoryTabSwitcher;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> GearTabButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ConsumableTabButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> MiscTabButton;

    //~ Gold
protected:
    void UpdateGold(const int64 Gold);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Gold_txt;
};
