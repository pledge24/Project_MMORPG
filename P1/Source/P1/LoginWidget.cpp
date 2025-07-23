// Fill out your copyright notice in the Description page of Project Settings.


#include "LoginWidget.h"
#include "P1.h"
#include "LoginModeBase.h"
#include "Components/EditableTextBox.h"   // ID, PW 입력란
#include "Components/Button.h"            // 로그인 버튼
#include "Components/TextBlock.h"         // 결과 출력 텍스트

// LoginWidget.cpp
void ULoginWidget::NativeConstruct()
{
    Super::NativeConstruct();

    LoginButton->OnClicked.AddDynamic(this, &ULoginWidget::OnLoginClicked);
    RegisterButton->OnClicked.AddDynamic(this, &ULoginWidget::OnRegisterClicked);
}

void ULoginWidget::OnLoginClicked()
{
    FString Username = UsernameBox->GetText().ToString();
    FString Password = PasswordBox->GetText().ToString();

    if (ALoginModeBase* Mode = Cast<ALoginModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        ULoginManager* Manager = Mode->GetLoginManager();
        if (Manager)
        {
            Manager->RequestLogin(Username, Password);
        }
    }
}

void ULoginWidget::OnRegisterClicked()
{
    FString Username = UsernameBox->GetText().ToString();
    FString Password = PasswordBox->GetText().ToString();

    if (ALoginModeBase* Mode = Cast<ALoginModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        ULoginManager* Manager = Mode->GetLoginManager();
        if (Manager)
        {
            Manager->RequestRegister(Username, Password);
        }
    }
}

void ULoginWidget::SetResultMessage(const FString& Message)
{
    ResultText->SetText(FText::FromString(Message));
}

void ULoginWidget::ClearMessage()
{
    UsernameBox->SetText(FText::GetEmpty());
    PasswordBox->SetText(FText::GetEmpty());
}
