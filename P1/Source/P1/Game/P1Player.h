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
	bool IsMyPlayer();

    virtual void Init(const Protocol::ObjectInfo& ObjectInfo);

    /** 상태 관련 */
	Protocol::MoveState GetMoveState() const { return ClientPos->state(); }
	void SetMoveState(Protocol::MoveState State);

    /** 이동 관련 */
	void SetClientPos(const Protocol::PosInfo& Info);
	void SetServerPos(const Protocol::PosInfo& Info);
	Protocol::PosInfo* GetPosInfo() const { return ClientPos; }

    /** 장착 관련*/
    void SetEquippedGear(const Protocol::Slot& Slot_);

    UFUNCTION(BlueprintImplementableEvent, Category = "Character")
    void ChangeMesh(int32 SlotId, int32 TemplateId);

    UFUNCTION(BlueprintImplementableEvent, Category = "Character")
    void SetName(const FText& ObjectName);

public:
    /** 델리게이트 모음 */
    //DECLARE_MULTICAST_DELEGATE_OneParam(OnEquippedGearChanged, const Protocol::Slot&);
    //OnEquippedGearChanged OnEquippedGearChanged;

private:
    void Move(float DeltaSeconds);
    FVector GetPerpendicularPoint() const;

protected:
    /** Weapon StaticMesh*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
    class UStaticMeshComponent* WeaponMesh;

    class Protocol::PosInfo* ClientPos;     // 클라이언트 위치(현재 캐릭터 위치)
	class Protocol::PosInfo* ServerPos;     // 서버로부터 수신받은 위치(Only Use Other Player)

    FText PlayerName;

private:
    FVector MoveDirection = FVector::ZeroVector;
    const float CorrectionThreshold = 100.f;
    const float CORRECTION_SPEED = 100.f;
    const float INTERP_SPEED = 5.f;
    const float RINTERP_SPEED = 5.f;

};
