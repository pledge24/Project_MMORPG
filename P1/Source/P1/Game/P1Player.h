// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Protocol.pb.h"
#include "P1Player.generated.h"

UCLASS()
class P1_API AP1Player : public ACharacter
{
	GENERATED_BODY()

public:
	AP1Player();

protected:
	virtual void BeginPlay();
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

public:
    UFUNCTION(BlueprintImplementableEvent, Category = "Character")
    void ChangeMesh(int32 SlotId, int32 TemplateId);

    UFUNCTION(BlueprintImplementableEvent, Category = "Character")
    void SetName(const FText& ObjectName);

    virtual void Init(const Protocol::ObjectInfo& ObjectInfo);

	bool IsMyPlayer();

    bool PushToMoveQueue(const Protocol::PosInfo& Info_);

    /** Setter함수 */
	void SetMoveState(Protocol::MoveState State);
	void SetClientPos(const Protocol::PosInfo& Info);
	void SetServerPos(const Protocol::PosInfo& Info);
    void SetEquippedGear(const Protocol::Slot& Slot_);

    /** Getter함수 */
	Protocol::MoveState GetMoveState() const { return ClientPos->state(); }
	Protocol::PosInfo* GetPosInfo() const { return ClientPos; }
    class UAttackSystemComponent* GetAttackSystemComponent() const { return AttackSystemComponent;}

private:
    /** 이동 관련 함수 */
    void Move(float DeltaSeconds);
    FVector FindPerpendicularPoint() const;

protected:
    /** Weapon CharacterMesh Component*/
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* WeaponMesh;

    /** Attack System Component*/
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UAttackSystemComponent* AttackSystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
    FText PlayerName;

    class Protocol::PosInfo* ClientPos;     // 클라이언트 위치(현재 캐릭터 위치)
	class Protocol::PosInfo* ServerPos;     // 서버로부터 수신받은 위치(Only Use Other Player)

private:
    TQueue<Protocol::PosInfo> MoveQueue;
    FVector MoveDirection = FVector::ZeroVector;
    const float CorrectionThreshold = 200.f;
    const float CORR_INTERP_SPEED = 5.f;
    const float CORR_RINTERP_SPEED = 5.f;
};
