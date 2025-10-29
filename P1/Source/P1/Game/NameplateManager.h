// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NameplateManager.generated.h"

class UNameplateWidget;

/**
 * 
 */
UCLASS()
class P1_API UNameplateManager : public ULocalPlayerSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; } // 에디터 틱 활성화 여부

public:
    bool AddOnActorNameplate(AActor* Actor, UNameplateWidget* NameplateWidget);
    bool RemoveOnActorNameplate(AActor* Actor);

    void Clear();

    void SetNameplateLocaction(AActor* Actor, UNameplateWidget* Nameplate);

private:
    UPROPERTY()
    TMap<AActor*, UNameplateWidget*> NameplateMappings;

    float WIDGET_OFFSET = -20.f;
};
