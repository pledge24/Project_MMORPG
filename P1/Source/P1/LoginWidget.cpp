// Fill out your copyright notice in the Description page of Project Settings.


#include "LoginWidget.h"
#include "P1.h"
#include "LoginModeBase.h"
#include "Components/EditableTextBox.h"   
#include "Components/Button.h"            
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"

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

    // 버튼들을 배열에 저장
    for (int32 i = 0; i < SlotButtonHorizontalBox->GetChildrenCount(); ++i)
    {
        if (UButton* Button = Cast<UButton>(SlotButtonHorizontalBox->GetChildAt(i)))
        {
            SlotButtons.Add(Button);
            Button->OnClicked.AddDynamic(this, &ULoginWidget::OnSlotButtonClicked);
        }
    }

    //SlotButtons.Empty();
    //ButtonToPanelIndex.Empty();

    //// HorizontalBox의 각 Canvas Panel에서 버튼 찾기
    //for (int32 PanelIndex = 0; PanelIndex < ButtonHorizontalBox->GetChildrenCount(); ++PanelIndex)
    //{
    //    if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(ButtonHorizontalBox->GetChildAt(PanelIndex)))
    //    {
    //        // Canvas Panel 내의 모든 버튼 찾기
    //        for (int32 i = 0; i < CanvasPanel->GetChildrenCount(); ++i)
    //        {
    //            if (UButton* Button = Cast<UButton>(CanvasPanel->GetChildAt(i)))
    //            {
    //                AllButtons.Add(Button);
    //                ButtonToPanelIndex.Add(PanelIndex);

    //                // 람다로 패널 인덱스와 버튼 인덱스 전달
    //                Button->OnClicked.AddLambda([this, PanelIndex, Button]()
    //                    {
    //                        OnButtonClickedInPanel(PanelIndex, Button);
    //                    });
    //            }
    //        }
    //    }
    //}
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

int32 ULoginWidget::GetClickedButtonIndex()
{
    for (int32 i = 0; i < SlotButtons.Num(); ++i)
    {
        if (SlotButtons[i]->IsPressed())
        {
            return i;
        }
    }
    return -1;
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

void ULoginWidget::OnSlotButtonClicked()
{
    int32 ClickedIndex = GetClickedButtonIndex();
    if (ClickedIndex != -1)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Button %d was clicked!"), ClickedIndex));
    }
}

