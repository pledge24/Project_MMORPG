#pragma once

#include "CoreMinimal.h"
#include "Characters/P1Creature.h"
#include "P1Player.generated.h"

UCLASS()
class P1_API AP1Player : public AP1Creature
{
    GENERATED_BODY()

public:
    AP1Player();

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay();
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;
    //~ End AActor Interface

    //~ Begin AP1Creature Interface
public:
    /** 서버가 보낸 오브젝트 정보로 초기화한다. 서버가 보낸 값으로만 부른다. */
    virtual void Initialize(const Protocol::EntityInfo& EntityInfo) override;

protected:
    virtual void S_Move(float DeltaSeconds) override;
    //~ End AP1Creature Interface

    //~ Equipment
public:
    /** 슬롯에 맞는 메시로 바꾼다. 구현은 블루프린트에 있다. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Character")
    void ChangeMesh(int32 SlotId, int32 TemplateId);

    void SetEquipmentSlot(const Protocol::Slot& InSlot);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UStaticMeshComponent> WeaponMesh;

    //~ Identity
public:
    void SetPlayerName(const FText& InName);
    FText GetPlayerName() const { return GetCreatureName(); }

    //~ Progression
public:
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnLevelUp, int32);
    FOnLevelUp OnLevelUp;
};
