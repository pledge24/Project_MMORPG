#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Entity.pb.h"
#include "P1Creature.generated.h"

/**
 * 월드에 보이는 크리처의 공통 기반.
 *
 * 플레이어와 몬스터가 함께 쓰는 전투, 사망, 이름표를 맡고, 서버가 알린 일을 화면의 크리처에 반영한다.
 */
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

    /** 로컬 플레이어가 조종하는 크리처면 true. */
    virtual bool IsMyPlayer() const { return false; }

    //~ Combat
public:
    virtual void S_NormalAttack(uint32 Combo, float Yaw);
    virtual void S_Hit(int64 Damage, int64 UpdatedHp);

    class UP1AttackSystemComponent* GetAttackSystemComponent() const { return AttackSystemComponent; }

    /** 공격 컴포넌트가 없으면 false. */
    bool IsAttacking() const;

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

    /** 같은 액터를 살려 다시 쓰고 OnRespawn을 알린다. 리스폰 위치로는 부르는 쪽이 먼저 옮겨 둔다. */
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
