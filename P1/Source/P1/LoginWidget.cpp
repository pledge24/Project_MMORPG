// Fill out your copyright notice in the Description page of Project Settings.


#include "LoginWidget.h"
#include "P1.h"
#include "LoginModeBase.h"
#include "Components/EditableTextBox.h"   
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/CanvasPanel.h"
#include "Styling/SlateBrush.h"
//#include "Slate/SlateBrush.h"
#include "Styling/SlateTypes.h"

// LoginWidget.cpp
void ULoginWidget::NativeConstruct()
{
    Super::NativeConstruct();

    LoginButton->OnClicked.AddDynamic(this, &ULoginWidget::OnLoginClicked);
    RegisterButton->OnClicked.AddDynamic(this, &ULoginWidget::OnRegisterClicked);

    if (WidgetSwitcher)
    {
        WidgetSwitcher->SetActiveWidgetIndex(0); // 0번 위젯으로 시작
    }

    if (SlotButtonHorizontalBox)
    {
        // Init Character Slot Buttons.
        SlotButtonsMap.Empty();

        // 버튼 배열에 추가
        SlotButtonsMap.Add(SlotButton0, 0);
        SlotButtonsMap.Add(SlotButton1, 1);
        SlotButtonsMap.Add(SlotButton2, 2);
        SlotButtonsMap.Add(SlotButton3, 3);

        SlotButton0->OnClicked.AddDynamic(this, &ULoginWidget::OnSlotButton0Clicked);
        SlotButton1->OnClicked.AddDynamic(this, &ULoginWidget::OnSlotButton1Clicked);
        SlotButton2->OnClicked.AddDynamic(this, &ULoginWidget::OnSlotButton2Clicked);
        SlotButton3->OnClicked.AddDynamic(this, &ULoginWidget::OnSlotButton3Clicked);
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

void ULoginWidget::SwitchToIndex(int32 Index)
{
    if (WidgetSwitcher && WidgetSwitcher->GetNumWidgets() > Index)
    {
        WidgetSwitcher->SetActiveWidgetIndex(Index);
    }
}

void ULoginWidget::HighlightClickedButton(UButton* ClickedButton)
{
    // 외곽선 초기화
    for (const auto& Pair : SlotButtonsMap)
    {
        UButton* Button = Pair.Key;
        if (Button)
        {
            FButtonStyle Style = Button->WidgetStyle;
            Style.Normal.Margin = FMargin(0.0f);
            Button->SetStyle(Style);
        }
    }

    // 클릭된 버튼 외곽선 적용
    if (ClickedButton)
    {
        FButtonStyle ClickedStyle = ClickedButton->WidgetStyle;
        ClickedStyle.Normal.Margin = FMargin(0.2f);
        ClickedButton->SetStyle(ClickedStyle);

        LastClickedButtonIdx = SlotButtonsMap[ClickedButton];

        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("Slot button %d clicked!"), LastClickedButtonIdx));
    }
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


