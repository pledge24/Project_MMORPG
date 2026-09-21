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
    virtual void Tick(float DeltaTime) override;
    //~ End AActor Interface

    //~ Initialization
public:
    /** 서버가 보낸 오브젝트 정보로 초기화한다. 서버가 보낸 값으로만 부른다. */
    virtual void Initialize(const Protocol::ObjectInfo& ObjectInfo);

    bool IsMyPlayer() const;

    //~ Movement
public:
    bool PushToMoveQueue(const Protocol::PosInfo& InInfo);

    void SetMoveState(Protocol::MoveState State);
    void SetClientPos(const Protocol::PosInfo& Info);
    void SetServerPos(const Protocol::PosInfo& Info);

    Protocol::MoveState GetMoveState() const { return ClientPos->state(); }
    TSharedPtr<Protocol::PosInfo> GetPosInfo() const { return ClientPos; }

    FVector FindPerpendicularPoint() const;

    virtual void S_Move(float DeltaSeconds);

protected:
    /** 클라이언트 위치다. 지금 화면에 보이는 캐릭터의 위치다. */
    TSharedPtr<Protocol::PosInfo> ClientPos;

    /** 서버에서 받은 위치다. 내 플레이어가 아닌 캐릭터에만 쓴다. */
    TSharedPtr<Protocol::PosInfo> ServerPos;

private:
    TQueue<Protocol::PosInfo> MoveQueue;
    FVector MoveDirection = FVector::ZeroVector;
    const float CorrectionMaxThreshold = 800.f;
    const float CORR_INTERP_SPEED = 5.f;
    const float CORR_RINTERP_SPEED = 5.f;

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
