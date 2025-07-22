// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Http.h"
#include "LoginWidget.generated.h"

/**
 * 
 */
UCLASS()
class P1_API ULoginWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 블루프린트에서 호출할 함수 */

    UFUNCTION(BlueprintCallable, Category = "Login")
    void RequestRegister(const FString& Username, const FString& Password);

    UFUNCTION(BlueprintCallable, Category = "Login")
    void RequestLogin(const FString& Username, const FString& Password);
};
