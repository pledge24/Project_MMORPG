#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1ShopWidget.generated.h"

class AP1MyPlayer;
class UP1SlotWidget;

UCLASS()
class P1_API UP1ShopWidget : public UUserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
public:
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Purchase
public:
    void BindMyPlayerSpawned(AP1MyPlayer* MyPlayer);

protected:
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendBuyItemPacket(UP1SlotWidget* Slot_);

    /** 구매 응답을 기다리는 동안 참이다. 중복 요청을 막는다. */
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    bool PendingPacket = false;
};
