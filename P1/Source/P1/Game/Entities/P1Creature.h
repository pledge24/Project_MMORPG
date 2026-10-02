#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Protocol.pb.h"
#include "P1Creature.generated.h"

UCLASS()
class P1_API AP1Creature : public ACharacter
{
    GENERATED_BODY()

public:
    AP1Creature();

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    //~ End AActor Interface

    //~ Initialization
public:
    /** 서버가 보낸 엔티티 정보로 초기화한다. 서버가 보낸 값으로만 부른다. */
    virtual void Initialize(const Protocol::EntityInfo& EntityInfo);

    bool IsMyPlayer() const;

    //~ Combat
public:
    virtual void S_NormalAttack(uint32 Combo, float Yaw);
    virtual void S_Hit(int64 Damage, int64 UpdatedHp);

    class UP1AttackSystemComponent* GetAttackSystemComponent() const { return AttackSystemComponent; }

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHit, const int64&, Damage, const int64&, UpdatedHp);

    UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Delegate")
    FOnHit OnHit;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UP1AttackSystemComponent> AttackSystemComponent;

    //~ Death
public:
    UFUNCTION(BlueprintCallable, Category = "Creature")
    void SetDeadState(bool IsDead);

    UFUNCTION(BlueprintCallable, Category = "Creature")
    bool IsDead() const { return _IsDead; }

    virtual void S_Die();

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDie, AActor*, KilledCreature);

    UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Delegate")
    FOnDie OnDie;

    /** 사망 상태를 풀고 OnRespawn을 알린다. 같은 액터를 다시 쓴다. 리스폰 위치는 부르는 쪽이 먼저 옮겨 둔다. */
    virtual void S_Respawn();

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRespawn, AActor*, RespawnedCreature);

    UPROPERTY(BlueprintAssignable, Category = "Delegate")
    FOnRespawn OnRespawn;

private:
    bool _IsDead = false;

    //~ Nameplate
public:
    void SetCreatureName(const FText& InName);
    FText GetCreatureName() const { return CreatureName; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
    FText CreatureName = FText::FromString("NULL");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UWidgetComponent> NameplateComponent;
};
