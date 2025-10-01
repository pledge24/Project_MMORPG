// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShopWidget.generated.h"

class USlotWidget;

/**
 * 
 */
UCLASS()
class P1_API UShopWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;

protected:
    UFUNCTION(BlueprintCallable, Category="Network")
    void SendBuyItemPacket(USlotWidget* _Slot);

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    bool PendingPacket = false;
};
