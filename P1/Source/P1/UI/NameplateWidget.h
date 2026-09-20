// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types.h"
#include "NameplateWidget.generated.h"

class AP1Player;
class AMonster;
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
    virtual void NativeConstruct() override;

public:
    UFUNCTION(BlueprintCallable)
    void InitializeWidget(AActor* Actor);

    void InitializePlayerNameplate(AP1Player* TargetPlayer);
    void InitializeMonsterNameplate(AMonster* TargetMonster);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* NameTextBlock;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
    UProgressBarWidget* HpBar;

private:
    ESlateVisibility NameTextInitVisibility;
    ESlateVisibility HpBarInitVisibility;
};
