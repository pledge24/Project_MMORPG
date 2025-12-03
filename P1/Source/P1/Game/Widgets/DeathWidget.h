// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1MyPlayer.h"
#include "DeathWidget.generated.h"

/**
 * 
 */
UCLASS()
class P1_API UDeathWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
    virtual void NativeConstruct() override;
    void BindMyPlayerSpawned(AP1MyPlayer* MyPlayer);

public:
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void StartCountdown();

    /** 이벤트 함수 */
    UFUNCTION()
    void OnMyPlayerDie(AActor* KilledCreature);

protected:
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendReturnByDeathPacket();

    UFUNCTION(BlueprintCallable, Category = "Network")
    void Test();
};
