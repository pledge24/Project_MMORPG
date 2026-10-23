#include "UI/Frontend/P1LoginWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "Auth/P1AuthClient.h"
#include "Core/P1EntryFlowSubsystem.h"

void UP1LoginWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    LoginButton->OnClicked.AddDynamic(this, &UP1LoginWidget::OnLoginButtonClicked);
    RegisterButton->OnClicked.AddDynamic(this, &UP1LoginWidget::OnRegisterButtonClicked);
    QuitButton->OnClicked.AddDynamic(this, &UP1LoginWidget::OnQuitButtonClicked);
}

void UP1LoginWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 응답 구독. 떼는 것은 NativeDestruct가 한다.
    if (UP1AuthClient* AuthClient = GetAuthClient())
        AuthClient->OnAuthResult.AddUObject(this, &UP1LoginWidget::SetResultText);

    // 게임 서버와 연결이 끊겨 돌아왔으면 그 사유를 보여 준다. 이 화면에 있는 동안 생긴 사유는 로그인 메뉴가 넘겨준다.
    UGameInstance* GameInstance = GetGameInstance();
    if (UP1EntryFlowSubsystem* EntryFlow = GameInstance ? GameInstance->GetSubsystem<UP1EntryFlowSubsystem>() : nullptr)
    {
        const FString Notice = EntryFlow->ConsumeLoginNotice();
        if (Notice.IsEmpty() == false)
            ShowNotice(Notice);
    }
}

void UP1LoginWidget::NativeDestruct()
{
    // 서브시스템은 레벨보다 오래 산다. 떼지 않으면 로그인 맵을 다시 열 때마다 죽은 위젯의 항목이 쌓인다.
    if (UP1AuthClient* AuthClient = GetAuthClient())
        AuthClient->RemoveListener(this);

    Super::NativeDestruct();
}

void UP1LoginWidget::ShowNotice(const FString& Notice)
{
    SetResultText(false, Notice);
}

UP1AuthClient* UP1LoginWidget::GetAuthClient() const
{
    UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UP1AuthClient>() : nullptr;
}

void UP1LoginWidget::SetResultText(bool bSuccess, const FString& Message)
{
    const FLinearColor Color = bSuccess ? FLinearColor::White : FLinearColor::Red;

    ResultText->SetColorAndOpacity(FSlateColor(Color));
    ResultText->SetText(FText::FromString(Message));
}

void UP1LoginWidget::LockButtonBriefly(UButton* Button, FTimerHandle& LockTimer)
{
    // 잠긴 동안에는 버튼이 눌리지 않으므로 타이머가 겹쳐 걸리지 않는다.
    Button->SetIsEnabled(false);
    GetWorld()->GetTimerManager().SetTimer(LockTimer, FTimerDelegate::CreateWeakLambda(Button, [Button]()
    {
        Button->SetIsEnabled(true);
    }), BUTTON_LOCK_SECONDS, false);
}

void UP1LoginWidget::OnLoginButtonClicked()
{
    if (UP1AuthClient* AuthClient = GetAuthClient())
        AuthClient->RequestLogin(UsernameBox->GetText().ToString(), PasswordBox->GetText().ToString());

    LockButtonBriefly(LoginButton, LoginButtonLockTimer);
}

void UP1LoginWidget::OnRegisterButtonClicked()
{
    if (UP1AuthClient* AuthClient = GetAuthClient())
        AuthClient->RequestRegister(UsernameBox->GetText().ToString(), PasswordBox->GetText().ToString());

    LockButtonBriefly(RegisterButton, RegisterButtonLockTimer);
}

void UP1LoginWidget::OnQuitButtonClicked()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
