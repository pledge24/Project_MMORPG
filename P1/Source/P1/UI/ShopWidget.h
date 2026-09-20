// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShopWidget.generated.h"

class AP1MyPlayer;
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

public:
    void BindMyPlayerSpawned(AP1MyPlayer* MyPlayer);

protected:
    UFUNCTION(BlueprintCallable, Category="Network")
    void SendBuyItemPacket(USlotWidget* Slot_);

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    bool PendingPacket = false;
};
