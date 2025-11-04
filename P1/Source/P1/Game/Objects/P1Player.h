// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Objects/Creature.h"
#include "P1Player.generated.h"


UCLASS()
class P1_API AP1Player : public ACreature
{
	GENERATED_BODY()

public:
	AP1Player();

protected:
	virtual void BeginPlay();
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

public:
    UFUNCTION(BlueprintImplementableEvent, Category = "Character")
    void ChangeMesh(int32 SlotId, int32 TemplateId);

    virtual void Initialize(const Protocol::ObjectInfo& ObjectInfo) override;

    /** Setter함수 */
    void SetEquippedGear(const Protocol::Slot& InSlot);
    void SetPlayerName(const FText& InName);

    /** Getter함수 */
    FText GetPlayerName() const { return GetCreatureName(); }

protected:
    /** 이동 관련 함수 */
    virtual void S_Move(float DeltaSeconds) override;


protected:
    /** Weapon CharacterMesh Component*/
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* WeaponMesh;

};
