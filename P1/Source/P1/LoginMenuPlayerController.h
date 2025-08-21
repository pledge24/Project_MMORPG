// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LoginMenuPlayerController.generated.h"

class ULoginManager;

/**
 * 
 */
UCLASS()
class P1_API ALoginMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    ALoginMenuPlayerController();
    ULoginManager* GetLoginManager() { return LoginManager; }

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> LoginMenuWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UI")
    UUserWidget* LoginMenuWidget;

private:
    ULoginManager* LoginManager;
};
