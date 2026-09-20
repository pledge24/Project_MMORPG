// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "P1LoginMenuPlayerController.generated.h"

class UP1LoginManager;

/**
 * 
 */
UCLASS()
class P1_API AP1LoginMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    AP1LoginMenuPlayerController();
    UP1LoginManager* GetLoginManager() { return LoginManager; }

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UP1LoginWidget> LoginMenuWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UI")
    UP1LoginWidget* LoginMenuWidget;

private:
    UPROPERTY()
    UP1LoginManager* LoginManager;
};
