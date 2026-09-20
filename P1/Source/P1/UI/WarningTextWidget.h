// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WarningTextWidget.generated.h"

/**
 * 
 */
UCLASS()
class P1_API UWarningTextWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;

public:
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UI")
    void DisplayWarningMessage(const FText& WarningMessage);

};
