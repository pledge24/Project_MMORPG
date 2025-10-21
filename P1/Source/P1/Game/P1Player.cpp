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
#include "P1.h"
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

	ClientPos = new Protocol::PosInfo();
	ServerPos = new Protocol::PosInfo();

    // Create a Weapon Static Mesh
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    USkeletalMeshComponent* CharacterMesh = GetMesh();

    if (WeaponMesh && CharacterMesh)
    {
        WeaponMesh->SetupAttachment(GetMesh(), FName("weapon_r"));
    }
}

void AP1Player::BeginPlay()
{
	Super::BeginPlay();

	{
		FVector Location = GetActorLocation();
        ServerPos->set_x(Location.X);
        ServerPos->set_y(Location.Y);
        ServerPos->set_z(Location.Z);
        ServerPos->set_yaw(GetControlRotation().Yaw);
        ServerPos->set_state(Protocol::MOVE_STATE_IDLE);

        ClientPos->CopyFrom(*ServerPos);
	}
}

void AP1Player::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    {
        delete ClientPos;
        delete ServerPos;

        ClientPos = nullptr;
        ServerPos = nullptr;
    }
}

void AP1Player::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

    // Cache: 틱마다 플레이어의 이전 틱 위치 정보 저장
    {
        FVector Location = GetActorLocation();
        ClientPos->set_x(Location.X);
        ClientPos->set_y(Location.Y);
        ClientPos->set_z(Location.Z);
        ClientPos->set_yaw(GetActorRotation().Yaw);
    }

    if (IsMyPlayer() == false)
    {
        Move(DeltaSeconds);
    }
}

bool AP1Player::IsMyPlayer()
{
	return Cast<AP1MyPlayer>(this) != nullptr;
}

void AP1Player::Init(const Protocol::ObjectInfo& ObjectInfo)
{
    if (ObjectInfo.player_info().equipped_gear().empty())
        return;

    // 장착한 장비를 메시로 표현
    for (const auto& Pair : ObjectInfo.player_info().equipped_gear())
    {
        const Protocol::Slot& Slot_ = Pair.second;
        SetEquippedGear(Slot_);
    }

    PlayerName = FText::FromString(UTF8_TO_TCHAR(ObjectInfo.player_info().name().c_str()));
    SetName(PlayerName);
}

void AP1Player::SetMoveState(Protocol::MoveState State)
{
	if (ClientPos->state() == State)
		return;

	ClientPos->set_state(State);
}

void AP1Player::SetClientPos(const Protocol::PosInfo& Info)
{
	if (ClientPos->object_id() != 0)
	{
		assert(SrcInfo->object_id() == Info.object_id());
	}

	ClientPos->CopyFrom(Info);

	FVector Location(Info.x(), Info.y(), Info.z());
	SetActorLocation(Location);
}

void AP1Player::SetServerPos(const Protocol::PosInfo& Info)
{
	if (ClientPos->object_id() != 0)
	{
		assert(ClientPos->object_id() == Info.object_id());
	}

    if (ServerPos == nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("ServerPos Is NullNullNull")));
        return;
    }

    // Dest에 최종 상태 복사
    ServerPos->CopyFrom(Info);

    // Set Move Direction
    MoveDirection = FRotator(0.f, ServerPos->yaw(), 0.f).Vector();

    // 이동 패킷 받자마자 state는 바로 세팅
    SetMoveState(Info.state());
}

void AP1Player::SetEquippedGear(const Protocol::Slot& Slot_)
{
    int32 SlotId = Slot_.slot_id();
    int32 TemplateId = Slot_.item().template_id();
    ChangeMesh(SlotId, TemplateId);
}

void AP1Player::Move(float DeltaSeconds)
{
    FVector ClientLocation = GetActorLocation();
    FVector ServerLocation = FVector(ServerPos->x(), ServerPos->y(), ServerPos->z());
    const float Dist = FVector::Distance(ClientLocation, ServerLocation);

    // Correction
    if (Dist > CorrectionThreshold) 
    {
        // 보정 거리 초과 시 Reposition
        SetActorLocation(ServerLocation);
        SetActorRotation(FRotator(0, ServerPos->yaw(), 0));
    }
    else
    {
        FVector CorrectionPoint = MoveDirection == FVector::Zero() ?
            ServerLocation : GetPerpendicularPoint();

        FVector CorrectedClientLocation = FMath::VInterpTo(ClientLocation, CorrectionPoint, DeltaSeconds, INTERP_SPEED);
        SetActorLocation(CorrectedClientLocation);
    }

    // Moving State
    if (ServerPos->state() == Protocol::MOVE_STATE_RUN)
    {
        AddMovementInput(MoveDirection);
    }

    // Idle State(제자리 회전 보정)
    /*if (ServerPos->state() == Protocol::MOVE_STATE_IDLE)
    {
        if (ServerPos->yaw() != GetActorRotation().Yaw)
        {
            FRotator TargetRot = FRotator(0, ServerPos->yaw(), 0);
            FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaSeconds, RINTERP_SPEED);

            SetActorRotation(NewRot);
        }
    }*/
}

FVector AP1Player::GetPerpendicularPoint() const
{
    FVector ServerPoint = FVector(ServerPos->x(), ServerPos->y(), ServerPos->z());
    FVector ClosestPoint = UKismetMathLibrary::FindClosestPointOnLine(GetActorLocation(), ServerPoint, MoveDirection);

    return ClosestPoint;
}
