// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1ShopWidget.generated.h"

class AP1MyPlayer;
class UP1SlotWidget;

/**
 * 
 */
UCLASS()
class P1_API UP1ShopWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;

public:
    void BindMyPlayerSpawned(AP1MyPlayer* MyPlayer);

protected:
    UFUNCTION(BlueprintCallable, Category="Network")
    void SendBuyItemPacket(UP1SlotWidget* Slot_);

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    bool PendingPacket = false;
};
