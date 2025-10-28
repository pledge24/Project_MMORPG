// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/NameplateManager.h"
#include "Blueprint/UserWidget.h"
#include "NameplateWidget.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "P1Player.h"
#include "NameplateWidget.h"
#include "Monster.h"
#include "Types.h"

void UNameplateManager::InitializeManager(APlayerController* PC, const TSubclassOf<UNameplateWidget>& WidgetClass)
{
    PlayerController = PC;
    NameplateWidgetClass = WidgetClass;
}

bool UNameplateManager::AddOnActorNameplate(AActor* Actor)
{
    if (!NameplateWidgetClass)
        return false;

    UNameplateWidget* NameplateWidget = CreateWidget<UNameplateWidget>(PlayerController, NameplateWidgetClass);
    if (NameplateWidget)
    {
        // TODO: 초기화 작업 추가
        if (Actor->IsA<AP1Player>())
        {
            NameplateWidget->SetNameplateMode(ECreatureType::Player);
        }
        else if (Actor->IsA<AMonster>())
        {
            NameplateWidget->SetNameplateMode(ECreatureType::Monster);
        }
        else
        {
            return false;
        }

        NameplateMappings.Add(Actor, NameplateWidget);
        NameplateWidget->AddToViewport();
    }
    
    return true;
}

bool UNameplateManager::RemoveOnActorNameplate(AActor* Actor)
{
    if (NameplateMappings.Contains(Actor) == false)
        return false;
    
    NameplateMappings.Remove(Actor);

    return true;
}

void UNameplateManager::Update()
{
    for (auto& Pair : NameplateMappings)
    {
        AActor* Actor = Pair.Key;
        UNameplateWidget* Nameplate = Pair.Value;

        if (IsValid(Actor) == false || IsValid(Nameplate) == false)
            continue;

        if (ACharacter* Character = Cast<ACharacter>(Actor))
        {
            UCapsuleComponent* CapsuleComponent = Character->GetCapsuleComponent();

            if (CapsuleComponent)
            {
                float HalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
                FVector WorldHeadPos = Character->GetActorLocation() + HalfHeight * 2.f;
                FVector2D ScreenHeadPos;

                bool bOnScreen = PlayerController->ProjectWorldLocationToScreen(WorldHeadPos, ScreenHeadPos);
                if (bOnScreen)
                {
                    FVector2D WidgetPos = ScreenHeadPos + FVector2D(0, WIDGET_OFFSET);
                    Nameplate->SetPositionInViewport(WidgetPos, false);
                    Nameplate->SetVisibility(ESlateVisibility::Visible);
                }
                else
                {
                    Nameplate->SetVisibility(ESlateVisibility::Hidden);
                }

            }
        }

    }

}

void UNameplateManager::Clear()
{
    if (NameplateMappings.Num() > 0)
        NameplateMappings.Empty();
}