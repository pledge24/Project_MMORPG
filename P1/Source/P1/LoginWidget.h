// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Button.h"    
#include "LoginWidget.generated.h"

/**
 * 
 */
UCLASS()
class P1_API ULoginWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /* 외부에서 발동되는 이벤트 */
    void OnRecvResult(bool success, const FString& Message);

    UFUNCTION(BlueprintImplementableEvent, Category = "CharacterSelect")
    void OnRecvCharacterOverviews(const TArray<FCharacterOverview>& Characters);

private:
    /* 버튼 클릭 이벤트 */
    UFUNCTION(BlueprintCallable, Category = "Login")
    void OnLoginClicked(FString Username, FString Password);

    UFUNCTION(BlueprintCallable, Category = "Login")
    void OnRegisterClicked(FString Username, FString Password);


protected:
    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "Login")
    UWidgetSwitcher* WidgetSwitcher;

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "Login")
    class UTextBlock* ResultText;

private:
    int32 LastClickedButtonIdx = -1;
};
