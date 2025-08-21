// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InGamePlayerController.generated.h"

/**
 * 
 */
UCLASS()
class P1_API AInGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    AInGamePlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

private:
    void ToggleStatusWindow();
    void ToggleInventory();
    void ToggleWidget(UUserWidget* Widget, int32 FlagIdx);

public:
    /** HUD UI */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> HUDWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UI")
    UUserWidget* HUDWidget;

    /** Control Help UI */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> HelpWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UI")
    UUserWidget* HelpWidget;

private:
    /** 상태창 UI*/
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> StatusWindowWidgetClass;

    UPROPERTY()
    UUserWidget* StatusWindowWidget;

    /** 인벤토리 UI*/
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> InventoryWidgetClass;

    UPROPERTY()
    UUserWidget* InventoryWidget;

private:
    int32 ToggleFlag = 0;
};
