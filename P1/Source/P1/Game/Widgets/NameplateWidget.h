// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types.h"
#include "NameplateWidget.generated.h"

/**
 * 
 */
UCLASS()
class P1_API UNameplateWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent)
    void SetNameplateMode(ECreatureType CreatureType);
};
