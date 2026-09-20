// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Button.h"    
#include "LoginWidget.generated.h"

namespace Protocol
{
    class S_LOGIN;
    class S_CREATE_CHARACTER;
    class S_DELETE_CHARACTER;
}

/**
 * 
 */
UCLASS()
class P1_API ULoginWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /* 외부 호출 함수 */
    void SetResultText(bool success, const FString& Message);

    void FetchCharacterOverviews(Protocol::S_LOGIN& pkt);
    void AddCharacterOverview(Protocol::S_CREATE_CHARACTER& pkt);
    void RemoveCharacterOverview(Protocol::S_DELETE_CHARACTER& pkt);

    /* C++ 호출 이벤트 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Character Create")
    void OnRecvCreateCharacterRes(bool Success, const FString& Cause, int64 CharacterId = -1);

protected:
    /** 네트워크 관련 함수 */
    UFUNCTION(BlueprintCallable, Category = "Login")
    void SendLoginRequest(FString Username, FString Password);

    UFUNCTION(BlueprintCallable, Category = "Login")
    void SendRegisterRequest(FString Username, FString Password);

    UFUNCTION(BlueprintCallable, Category = "Character Select")
    void SendEnterGamePkt();

    UFUNCTION(BlueprintCallable, Category = "Character Create")
    void SendCreateCharacterPkt(FString CharacterName, int32 CharacterClassId);

    UFUNCTION(BlueprintCallable, Category = "Character Delete")
    void SendDeleteCharacterPkt();

    UFUNCTION(BlueprintImplementableEvent, Category = "Character Select")
    void OnDisplayCharacterOverviews(const TArray<FCharacterOverview>& Characters);

protected:
    /** BindWidget */
    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "Login")
    UWidgetSwitcher* WidgetSwitcher;

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "Login")
    class UTextBlock* ResultText;

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "Character Create")
    class UTextBlock* CC_DescriptionText;

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "Character Create")
    class UEditableTextBox* CC_CharacterNameText;

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "Character Create")
    int32 CC_CharacterClassId;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Character Select")
    TArray<FCharacterOverview> CharacterOverviews;

    UPROPERTY(BlueprintReadWrite, Category = "Character Select")
    int32 LastClickedSlotIdx = -1;
};
