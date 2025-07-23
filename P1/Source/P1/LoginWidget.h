// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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

    void SetResultMessage(const FString& Message);

    void ClearMessage();

private:
    UFUNCTION()
    void OnLoginClicked();

    UFUNCTION()
    void OnRegisterClicked();

    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* UsernameBox;

    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* PasswordBox;

    UPROPERTY(meta = (BindWidget))
    class UButton* LoginButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* RegisterButton;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ResultText;	
};
