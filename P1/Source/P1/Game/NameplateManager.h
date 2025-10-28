// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NameplateManager.generated.h"

class UNameplateWidget;

/**
 * 
 */
UCLASS()
class P1_API UNameplateManager : public UObject
{
	GENERATED_BODY()

public:
    void InitializeManager(APlayerController* PC, const TSubclassOf<UNameplateWidget>& WidgetClass);

    bool AddOnActorNameplate(AActor* Actor);
    bool RemoveOnActorNameplate(AActor* Actor);

    void Update();
    void Clear();

private:
    UPROPERTY()
    TSubclassOf<UNameplateWidget> NameplateWidgetClass;

    UPROPERTY()
    TMap<AActor*, UNameplateWidget*> NameplateMappings;

    UPROPERTY()
    APlayerController* PlayerController;

    float WIDGET_OFFSET = -20.f;
};
