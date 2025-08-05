// Fill out your copyright notice in the Description page of Project Settings.


#include "LoginWidget.h"
#include "P1.h"
#include "LoginModeBase.h"
#include "Components/TextBlock.h"

void ULoginWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (WidgetSwitcher)
    {
        WidgetSwitcher->SetActiveWidgetIndex(0); // 0번 위젯으로 시작
    }
}

void ULoginWidget::OnRecvResult(bool bSuccess, const FString& Message)
{
    FLinearColor Color = bSuccess ? FLinearColor::White : FLinearColor::Red;

    ResultText->SetColorAndOpacity(FSlateColor(Color));
    ResultText->SetText(FText::FromString(Message));
}

//void ULoginWidget::OnRecvCharacterOverviews(TArray<FCharacterOverview>& Characters)
//{
//    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("OnRecvCharacterOverviews")));
//}

void ULoginWidget::OnLoginClicked(FString Username, FString Password)
{
    if (ALoginModeBase* Mode = Cast<ALoginModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        ULoginManager* Manager = Mode->GetLoginManager();
        if (Manager)
        {
            Manager->RequestLogin(Username, Password);
        }
    }
}

void ULoginWidget::OnRegisterClicked(FString Username, FString Password)
{
    if (ALoginModeBase* Mode = Cast<ALoginModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        ULoginManager* Manager = Mode->GetLoginManager();
        if (Manager)
        {
            Manager->RequestRegister(Username, Password);
        }
    }
}


