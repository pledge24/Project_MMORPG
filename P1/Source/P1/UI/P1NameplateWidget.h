// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Utils/Types.h"
#include "P1NameplateWidget.generated.h"

class AP1Player;
class AP1Monster;
class UTextBlock;
class UP1ProgressBarWidget;

/**
 * 
 */
UCLASS()
class P1_API UP1NameplateWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

public:
    UFUNCTION(BlueprintCallable)
    void InitializeWidget(AActor* Actor);

    void InitializePlayerNameplate(AP1Player* TargetPlayer);
    void InitializeMonsterNameplate(AP1Monster* TargetMonster);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* NameTextBlock;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
    UP1ProgressBarWidget* HpBar;

private:
    ESlateVisibility NameTextInitVisibility;
    ESlateVisibility HpBarInitVisibility;
};
