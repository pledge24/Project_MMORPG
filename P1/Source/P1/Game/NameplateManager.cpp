// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/NameplateManager.h"
#include "Blueprint/UserWidget.h"
#include "NameplateWidget.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "NameplateWidget.h"
#include "InGamePlayerController.h"


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

bool UNameplateManager::RegisterNameplate(AActor* Actor, UNameplateWidget* NameplateWidget)
{
    if (!Actor || !NameplateWidget)
        return false;

    NameplateMappings.Add(Actor, NameplateWidget);

    NameplateWidget->InitializeWidget(Actor);
    SetNameplateLocaction(Actor, NameplateWidget);
    NameplateWidget->AddToViewport();

    return true;
}

bool UNameplateManager::UnRegisterNameplate(AActor* Actor)
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
            FVector WorldLoc = Character->GetActorLocation() + FVector(0, 0, HalfHeight);
            FVector2D ScreenLoc;

            bool bOnScreen = PlayerController->ProjectWorldLocationToScreen(WorldLoc, ScreenLoc);
            if (bOnScreen)
            {
                Nameplate->SetPositionInViewport(ScreenLoc);
                Nameplate->SetVisibility(ESlateVisibility::Visible);
            }
            else
            {
                Nameplate->SetVisibility(ESlateVisibility::Hidden);
            }
        }
    }
}