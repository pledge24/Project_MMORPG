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

    void SetResultMessage(const FString& Message);
    void ClearMessage();

    UFUNCTION(BlueprintCallable)
    void SwitchToIndex(int32 Index);

    void HighlightClickedButton(UButton* ClickedButton);

private:
    int32 LastClickedButtonIdx = -1;

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

    /* 캐릭터 슬롯 관련 */
    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* SlotButtonHorizontalBox;

    UPROPERTY()
    TMap<UButton*, int32> SlotButtonsMap;

    /* 캐릭터 슬롯 관련(시도) */
    UPROPERTY(meta = (BindWidget))
    UButton* SlotButton0;

    UPROPERTY(meta = (BindWidget))
    UButton* SlotButton1;

    UPROPERTY(meta = (BindWidget))
    UButton* SlotButton2;

    UPROPERTY(meta = (BindWidget))
    UButton* SlotButton3;

    
private:
    /* Event Function */
    UFUNCTION()
    void OnLoginClicked();

    UFUNCTION()
    void OnRegisterClicked();

    UFUNCTION()
    void OnSlotButton0Clicked() { HighlightClickedButton(SlotButton0); }

    UFUNCTION()
    void OnSlotButton1Clicked() { HighlightClickedButton(SlotButton1); }

    UFUNCTION()
    void OnSlotButton2Clicked() { HighlightClickedButton(SlotButton2); }

    UFUNCTION()
    void OnSlotButton3Clicked() { HighlightClickedButton(SlotButton3); }

};
