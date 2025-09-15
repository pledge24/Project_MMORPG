// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/P1Player.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "P1MyPlayer.h"

AP1Player::AP1Player()
{
	//============================import by TPS=========================
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	//====================================================================

	SrcInfo = new Protocol::PosInfo();
	DestInfo = new Protocol::PosInfo();
}

AP1Player::~AP1Player()
{
	delete SrcInfo;
	delete DestInfo;
	SrcInfo = nullptr;
	DestInfo = nullptr;
}

void AP1Player::BeginPlay()
{
	Super::BeginPlay();

	{
		FVector Location = GetActorLocation();
		DestInfo->set_x(Location.X);
		DestInfo->set_y(Location.Y);
		DestInfo->set_z(Location.Z);
		DestInfo->set_yaw(GetControlRotation().Yaw);

		//SetMoveState(Protocol::MOVE_STATE_IDLE);
	}
}

void AP1Player::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 틱마다 플레이어의 위치를 수집해서 PlayerInfo에 저장
	{
		FVector Location = GetActorLocation();
		SrcInfo->set_x(Location.X);
		SrcInfo->set_y(Location.Y);
		SrcInfo->set_z(Location.Z);
		SrcInfo->set_yaw(GetControlRotation().Yaw);
	}

	if (IsMyPlayer() == false)
	{
		// 이렇게 하면 안된다?
		/*FVector Location = GetActorLocation();
		FVector DestLocation = FVector(DestInfo->x(), DestInfo->y(), DestInfo->z());

		FVector MoveDir = (DestLocation - Location);
		const float DistToDest = MoveDir.Length();
		MoveDir.Normalize();

		float MoveDist = (MoveDir * 600.f * DeltaSeconds).Length();
		MoveDist = FMath::Min(MoveDist, DistToDest);
		FVector NextLocation = Location + MoveDir* MoveDist;

		SetActorLocation(NextLocation);*/

		// TEST: 수신된 패킷에서 방향만 사용.
		const Protocol::MoveState State = SrcInfo->state();

		if (State == Protocol::MOVE_STATE_RUN)
		{
			SetActorRotation(FRotator(0, DestInfo->yaw(), 0));
			AddMovementInput(GetActorForwardVector());
		}
		else
		{

		}
	}
}

bool AP1Player::IsMyPlayer()
{
	return Cast<AP1MyPlayer>(this) != nullptr;
}

void AP1Player::Init(const Protocol::ObjectInfo& ObjectInfo)
{
    // 장착한 장비를 메시로 표현
    for (auto& _Slot : ObjectInfo.player_info().equipped_gear())
    {
        SetEquippedGear(_Slot);
    }
}

void AP1Player::SetMoveState(Protocol::MoveState State)
{
	if (SrcInfo->state() == State)
		return;

	SrcInfo->set_state(State);

	// TODO
}

void AP1Player::SetPosInfo(const Protocol::PosInfo& Info)
{
	if (SrcInfo->object_id() != 0)
	{
		assert(SrcInfo->object_id() == Info.object_id());
	}

	SrcInfo->CopyFrom(Info);

	FVector Location(Info.x(), Info.y(), Info.z());
	SetActorLocation(Location);
}

void AP1Player::SetDestInfo(const Protocol::PosInfo& Info)
{
	if (SrcInfo->object_id() != 0)
	{
		assert(SrcInfo->object_id() == Info.object_id());
	}

	// Dest에 최종 상태 복사
	DestInfo->CopyFrom(Info);

	// 상태만 바로 관리하자.
	SetMoveState(Info.state());
}

void AP1Player::SetEquippedGear(const Protocol::Slot& _Slot)
{
    int32 SlotId = _Slot.slot_id();
    int32 TemplateId = _Slot.item().template_id();
    ChangeMesh(SlotId, TemplateId);
}
