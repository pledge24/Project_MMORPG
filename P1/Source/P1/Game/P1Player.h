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
	virtual ~AP1Player();

protected:
	virtual void BeginPlay();
	virtual void Tick(float DeltaSeconds) override;

public:
	bool IsMyPlayer();

    virtual void Init(const Protocol::ObjectInfo& ObjectInfo);

    /** 상태 관련 */
	Protocol::MoveState GetMoveState() { return PlayerInfo->state(); }
	void SetMoveState(Protocol::MoveState State);

    /** 이동 관련 */
	void SetPosInfo(const Protocol::PosInfo& Info);
	void SetDestInfo(const Protocol::PosInfo& Info);
	Protocol::PosInfo* GetPlayerInfo() { return PlayerInfo; }

    /** 장착 관련*/
    void UpdateEquippedGear(const Protocol::Slot& _Slot);
    UFUNCTION(BlueprintImplementableEvent, Category = "Character")
    void OnChangeMesh(int32 SlotId, int32 TemplateId);

protected:
    class Protocol::PosInfo* PlayerInfo; // 현재 위치
	class Protocol::PosInfo* DestInfo; // 목적지
};
