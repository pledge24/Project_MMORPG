// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types.h"
#include "NameplateWidget.generated.h"

class UTextBlock;
class UProgressBarWidget;

/**
 * 
 */
UCLASS()
class P1_API UNameplateWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    void NativeConstruct() override;
    void InitializeWidget(AActor* Actor);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* NameTextBlock;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
    UProgressBarWidget* HpBar;

private:
    ESlateVisibility NameTextInitVisibility;
    ESlateVisibility HpBarInitVisibility;
};
