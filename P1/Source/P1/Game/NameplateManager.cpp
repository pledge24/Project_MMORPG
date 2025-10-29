// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/NameplateManager.h"
#include "Blueprint/UserWidget.h"
#include "NameplateWidget.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "NameplateWidget.h"
#include "InGamePlayerController.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UNameplateManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UNameplateManager::Deinitialize()
{
    Super::Deinitialize();
}

void UNameplateManager::Tick(float DeltaTime)
{
    for (auto& Pair : NameplateMappings)
    {
        AActor* Actor = Pair.Key;
        UNameplateWidget* Nameplate = Pair.Value;

        if (IsValid(Actor) == false || IsValid(Nameplate) == false)
            continue;

        SetNameplateLocaction(Actor, Nameplate);
    }
}

TStatId UNameplateManager::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UMyLocalPlayerSubsystem, STATGROUP_Tickables);
}

bool UNameplateManager::AddOnActorNameplate(AActor* Actor, UNameplateWidget* NameplateWidget)
{
    if (!Actor || !NameplateWidget)
        return false;

    NameplateWidget->InitializeWidget(Actor);

    NameplateMappings.Add(Actor, NameplateWidget);
    SetNameplateLocaction(Actor, NameplateWidget);
    NameplateWidget->AddToViewport();

    return true;
}

bool UNameplateManager::RemoveOnActorNameplate(AActor* Actor)
{
    if (!Actor)
        return false;

    if (NameplateMappings.Contains(Actor) == false)
        return false;

    UNameplateWidget* NameplateWidget = NameplateMappings[Actor];
    if (IsValid(NameplateWidget))
    {
        NameplateWidget->RemoveFromParent();
        NameplateMappings.Remove(Actor);
    }

    return true;
}

void UNameplateManager::Clear()
{
    if (NameplateMappings.Num() > 0)
        NameplateMappings.Empty();
}

void UNameplateManager::SetNameplateLocaction(AActor* Actor, UNameplateWidget* Nameplate)
{
    AInGamePlayerController* PlayerController = Cast<AInGamePlayerController>(GetWorld()->GetFirstPlayerController());
    if (!PlayerController)
        return;

    if (ACharacter* Character = Cast<ACharacter>(Actor))
    {
        UCapsuleComponent* CapsuleComponent = Character->GetCapsuleComponent();
        if (CapsuleComponent)
        {
            float HalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
            FVector WorldHeadPos = Character->GetActorLocation() + FVector(0, 0, HalfHeight * 2.f);

            FVector2D ScreenHeadPos;
            bool bOnScreen = PlayerController->ProjectWorldLocationToScreen(WorldHeadPos, ScreenHeadPos);

            if (bOnScreen)
            {
                // Alignment 제거 - 이게 문제를 일으킬 수 있음
                Nameplate->SetAnchorsInViewport(FAnchors(0.f, 0.f, 0.f, 0.f));
                // Nameplate->SetAlignmentInViewport(FVector2D(0.5f, 1.0f)); // 이 줄 제거

                FVector2D WidgetPos = ScreenHeadPos + FVector2D(0, WIDGET_OFFSET);
                Nameplate->SetPositionInViewport(WidgetPos, false);
                Nameplate->SetVisibility(ESlateVisibility::Visible);

                UE_LOG(LogTemp, Log, TEXT("WorldPos: %s, ScreenPos: %s, WidgetPos: %s"),
                    *WorldHeadPos.ToString(), *ScreenHeadPos.ToString(), *WidgetPos.ToString());
            }
            else
            {
                Nameplate->SetVisibility(ESlateVisibility::Hidden);
            }
        }
    }
}