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

protected:
    UFUNCTION(BlueprintCallable, Category="Network")
    void SendBuyPacket(USlotWidget* _Slot);
};
