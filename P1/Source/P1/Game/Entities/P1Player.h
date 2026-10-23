#pragma once

#include "CoreMinimal.h"
#include "Game/Entities/P1Creature.h"
#include "P1Player.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UP1GearAppearanceComponent;

/**
 * 월드에 보이는 플레이어 캐릭터.
 *
 * 내 플레이어와 다른 플레이어가 함께 쓰는 기반으로, 장착한 장비의 외형을 보여 준다.
 */
UCLASS()
class P1_API AP1Player : public AP1Creature
{
    GENERATED_BODY()

public:
    AP1Player();

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PostInitializeComponents() override;
    //~ End AActor Interface

    //~ Begin AP1Creature Interface
public:
    virtual void Initialize(const Protocol::EntityInfo& EntityInfo) override;
    //~ End AP1Creature Interface

    //~ Equipment
public:
    /** 장비 부위(GearPart)에 아이템 외형을 입힌다. TemplateId가 0이면 그 부위를 비운다. */
    void ApplyGear(int32 GearType, int32 TemplateId);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> WeaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> HelmetMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> ChestMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> LegsMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> ArmsMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> BootsMesh;

    /** 부위별 메시 컴포넌트를 장착한 아이템의 메시로 바꾼다. 에셋 테이블은 여기서 지정한다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UP1GearAppearanceComponent> GearAppearance;

    //~ Identity
public:
    void SetPlayerName(const FText& InName);
    FText GetPlayerName() const { return GetCreatureName(); }
};
