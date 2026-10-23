#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1LoginWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class UP1AuthClient;

/**
 * 로그인 화면.
 *
 * 아이디와 비밀번호로 로그인과 회원가입을 요청하고, 그 결과와 게임 서버에 접속하지 못한 사유를 보여 준다.
 */
UCLASS()
class P1_API UP1LoginWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;

    /** 인증 클라이언트와 접속 흐름의 델리게이트에서 이 위젯을 뗀다. */
    virtual void NativeDestruct() override;
    //~ End UUserWidget Interface

    //~ Login
public:
    /** 게임 서버에 접속하지 못했거나 연결이 끊긴 사유를 결과 문구 자리에 보여 준다. */
    void ShowNotice(const FString& Notice);

private:
    /** 게임 인스턴스가 없으면 nullptr. */
    UP1AuthClient* GetAuthClient() const;

    void SetResultText(bool bSuccess, const FString& Message);

    /** 요청이 겹치지 않게 버튼을 BUTTON_LOCK_SECONDS 동안 잠근다. */
    void LockButtonBriefly(UButton* Button, FTimerHandle& LockTimer);

    UFUNCTION()
    void OnLoginButtonClicked();

    UFUNCTION()
    void OnRegisterButtonClicked();

    UFUNCTION()
    void OnQuitButtonClicked();

    static constexpr float BUTTON_LOCK_SECONDS = 2.0f;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> UsernameBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> PasswordBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> LoginButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RegisterButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> QuitButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ResultText;

    FTimerHandle LoginButtonLockTimer;
    FTimerHandle RegisterButtonLockTimer;
};
