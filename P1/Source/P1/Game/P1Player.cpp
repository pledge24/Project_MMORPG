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
#include "PlayerInfoComponent.h"

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

	PlayerInfo = new Protocol::PosInfo();
	DestInfo = new Protocol::PosInfo();

    SetupPlayerInfoComponent();
}

AP1Player::~AP1Player()
{
	delete PlayerInfo;
	delete DestInfo;
	PlayerInfo = nullptr;
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
		PlayerInfo->set_x(Location.X);
		PlayerInfo->set_y(Location.Y);
		PlayerInfo->set_z(Location.Z);
		PlayerInfo->set_yaw(GetControlRotation().Yaw);
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
		const Protocol::MoveState State = PlayerInfo->state();

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

void AP1Player::SetPlayerData(const Protocol::ObjectInfo& ObjectInfo)
{
    //const google::protobuf::RepeatedPtrField<Protocol::Item>& source_items
    //    = ObjectInfo.player_info().equipment();

    //// 방법 1. TArray를 사용한다.
    //{
    //    TArray<Protocol::Item> Equipment;
    //    Equipment.Empty();
    //    
    //    for (auto&& item : source_items)
    //    {
    //        Equipment.Add(std::move(item));
    //    }
    //}

    //// 방법 2. Protobuf 방식을 유지한다.
    //{
    //    google::protobuf::RepeatedPtrField<Protocol::Item> Equipment;
    //    Equipment.CopyFrom(source_items);
    //    // Equipment.MergeFrom(source_items); // 존재하는 데이터 뒤에 붙이는 방식(Append)
    //}
}

void AP1Player::SetMoveState(Protocol::MoveState State)
{
	if (PlayerInfo->state() == State)
		return;

	PlayerInfo->set_state(State);

	// TODO
}

void AP1Player::SetPosInfo(const Protocol::PosInfo& Info)
{
	if (PlayerInfo->object_id() != 0)
	{
		assert(PlayerInfo->object_id() == Info.object_id());
	}

	PlayerInfo->CopyFrom(Info);

	FVector Location(Info.x(), Info.y(), Info.z());
	SetActorLocation(Location);
}

void AP1Player::SetDestInfo(const Protocol::PosInfo& Info)
{
	if (PlayerInfo->object_id() != 0)
	{
		assert(PlayerInfo->object_id() == Info.object_id());
	}

	// Dest에 최종 상태 복사
	DestInfo->CopyFrom(Info);

	// 상태만 바로 관리하자.
	SetMoveState(Info.state());
}

void AP1Player::SetupPlayerInfoComponent()
{
    PlayerInfoComponent = CreateDefaultSubobject<UPlayerInfoComponent>(TEXT("PlayerInfoComponent"));
}

