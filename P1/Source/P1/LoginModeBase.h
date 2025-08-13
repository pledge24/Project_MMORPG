// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LoginManager.h"
#include "GameFramework/GameModeBase.h"
#include "LoginModeBase.generated.h"

/**
 * 
 */
UCLASS()
class P1_API ALoginModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    virtual void StartPlay() override;
    ULoginManager* GetLoginManager() { return LoginManager; }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Login")
    TSubclassOf<ULoginWidget> LoginWidgetClass;

private:
    UPROPERTY()
    class ULoginManager* LoginManager;
};
