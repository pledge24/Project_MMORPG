// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Protocol.pb.h"
#include "Creature.generated.h"

UCLASS()
class P1_API ACreature : public ACharacter
{
	GENERATED_BODY()

public:
	ACreature();

protected:
	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

    
public:
    virtual void Initialize(const Protocol::ObjectInfo& ObjectInfo);    // Server Only

    bool IsMyPlayer() const;
    bool PushToMoveQueue(const Protocol::PosInfo& InInfo);

    /** Setter함수 */
    void SetMoveState(Protocol::MoveState State);
    void SetClientPos(const Protocol::PosInfo& Info);
    void SetServerPos(const Protocol::PosInfo& Info);
    void SetCreatureName(const FText& InName);

    /** Getter함수 */
    Protocol::MoveState GetMoveState() const { return ClientPos->state(); }
    class UAttackSystemComponent* GetAttackSystemComponent() const { return AttackSystemComponent; }
    Protocol::PosInfo* GetPosInfo() const { return ClientPos; }
    FText GetCreatureName() const { return CreatureName; }

public:
    /** 이동 관련 함수 */
    virtual void S_Move(float DeltaSeconds);
    virtual void S_NormalAttack(uint32 Combo);

    FVector FindPerpendicularPoint() const;

public:
    /** 델리게이트 */
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnStatInfoChanged, const Protocol::StatInfo&);
    FOnStatInfoChanged OnStatInfoChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnHpChanged, const int32&);
    FOnHpChanged OnHpChanged;

protected:
    /** Attack System Component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UAttackSystemComponent* AttackSystemComponent;

    /** Attack System Component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UWidgetComponent* NameplateComponent;

    /** Etc Data */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
    FText CreatureName = FText::FromString("NULL");

    class Protocol::PosInfo* ClientPos;     // 클라이언트 위치(현재 캐릭터 위치)
    class Protocol::PosInfo* ServerPos;     // 서버로부터 수신받은 위치(Only Use Other Player)

private:
    TQueue<Protocol::PosInfo> MoveQueue;
    FVector MoveDirection = FVector::ZeroVector;
    const float CorrectionThreshold = 200.f;
    const float CORR_INTERP_SPEED = 5.f;
    const float CORR_RINTERP_SPEED = 5.f;
};
