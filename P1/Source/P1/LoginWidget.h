// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetSwitcher.h"
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

    UFUNCTION(BlueprintCallable)
    void SwitchToIndex(int32 Index);

    int32 GetClickedButtonIndex();

private:
    /* Event Function */
    UFUNCTION()
    void OnLoginClicked();

    UFUNCTION()
    void OnRegisterClicked();

    UFUNCTION()
    void OnSlotButtonClicked();

protected:
    UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
    UWidgetSwitcher* WidgetSwitcher;

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

    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* SlotButtonHorizontalBox;

    UPROPERTY()
    TArray<UButton*> SlotButtons;
};
