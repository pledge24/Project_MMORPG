#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Button.h"
#include "P1LoginWidget.generated.h"

namespace Protocol
{
    class S_LOGIN;
    class S_CREATE_CHARACTER;
    class S_DELETE_CHARACTER;
}

UCLASS()
class P1_API UP1LoginWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
public:
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Screen
protected:
    /** 로그인 화면과 캐릭터 화면을 이 스위처로 오간다. */
    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "Login")
    TObjectPtr<UWidgetSwitcher> WidgetSwitcher;

    //~ Login
public:
    void SetResultText(bool success, const FString& Message);

protected:
    UFUNCTION(BlueprintCallable, Category = "Login")
    void SendLoginRequest(FString Username, FString Password);

    UFUNCTION(BlueprintCallable, Category = "Login")
    void SendRegisterRequest(FString Username, FString Password);

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "Login")
    TObjectPtr<class UTextBlock> ResultText;

    //~ Character Select
public:
    void FetchCharacterOverviews(Protocol::S_LOGIN& pkt);

protected:
    UFUNCTION(BlueprintCallable, Category = "Character Select")
    void SendEnterGamePkt();

    /** 캐릭터 목록을 화면에 그린다. 구현은 블루프린트에 있다. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Character Select")
    void OnDisplayCharacterOverviews(const TArray<FP1CharacterOverview>& Characters);

    UPROPERTY(BlueprintReadOnly, Category = "Character Select")
    TArray<FP1CharacterOverview> CharacterOverviews;

    /** 고른 슬롯이 없으면 -1이다. */
    UPROPERTY(BlueprintReadWrite, Category = "Character Select")
    int32 LastClickedSlotIdx = -1;

    //~ Character Create
public:
    void AddCharacterOverview(Protocol::S_CREATE_CHARACTER& pkt);

    /** 생성 응답을 블루프린트에 전달한다. 구현은 블루프린트에 있다. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Character Create")
    void OnRecvCreateCharacterRes(bool Success, const FString& Cause, int64 CharacterId = -1);

protected:
    UFUNCTION(BlueprintCallable, Category = "Character Create")
    void SendCreateCharacterPkt(FString CharacterName, int32 CharacterClassId);

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "Character Create")
    TObjectPtr<class UTextBlock> CC_DescriptionText;

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "Character Create")
    TObjectPtr<class UEditableTextBox> CC_CharacterNameText;

    UPROPERTY(meta = (BindWidget), BlueprintReadWrite, Category = "Character Create")
    int32 CC_CharacterClassId;

    //~ Character Delete
public:
    void RemoveCharacterOverview(Protocol::S_DELETE_CHARACTER& pkt);

protected:
    UFUNCTION(BlueprintCallable, Category = "Character Delete")
    void SendDeleteCharacterPkt();
};
