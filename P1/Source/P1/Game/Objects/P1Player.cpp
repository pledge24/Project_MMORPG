// Fill out your copyright notice in the Description page of Project Settings.


#include "P1Player.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "AttackSystemComponent.h"
#include "P1.h"
#include "P1MyPlayer.h"
#include "Log/LogCategory.h"

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
}

void AP1Player::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AP1Player::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AP1Player::Initialize(const Protocol::ObjectInfo& ObjectInfo)
{
    Super::Initialize(ObjectInfo);

    if (ObjectInfo.player_info().equipped_gear().empty())
        return;

    // 장착한 장비를 메시로 표현
    for (const auto& Pair : ObjectInfo.player_info().equipped_gear())
    {
        const Protocol::Slot& Slot_ = Pair.second;
        SetEquippedGear(Slot_);
    }
}

void AP1Player::SetEquippedGear(const Protocol::Slot& InSlot)
{
    int32 SlotId = InSlot.slot_id();
    int32 TemplateId = InSlot.item().template_id();
    ChangeMesh(SlotId, TemplateId);
}

void AP1Player::SetPlayerName(const FText& InName)
{
    SetCreatureName(InName);
}

void AP1Player::S_Move(float DeltaSeconds)
{
    Super::S_Move(DeltaSeconds);
}
