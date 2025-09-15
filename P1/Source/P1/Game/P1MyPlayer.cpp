// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/P1MyPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "P1.h"
#include "Kismet/KismetMathLibrary.h"
#include "Inventory.h"
#include "EquippedGear.h"

AP1MyPlayer::AP1MyPlayer()
{
    // Create a camera boom (pulls in towards the player if there is a collision)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
    CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
    FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

    // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
    // are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

    _PlayerInfo = new Protocol::PlayerInfo();
    _StatInfo = _PlayerInfo->mutable_stat_info();

    InventoryHelper = CreateDefaultSubobject<UInventory>(TEXT("InventoryComponent"));
    EquippedGearHelper = CreateDefaultSubobject<UEquippedGear>(TEXT("EquippedGearComponent"));
}

AP1MyPlayer::~AP1MyPlayer()
{
    delete _PlayerInfo;
    _PlayerInfo = nullptr;
    _StatInfo = nullptr;
}

void AP1MyPlayer::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AP1MyPlayer::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AP1MyPlayer::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AP1MyPlayer::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AP1MyPlayer::Look);
	}

}

void AP1MyPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Send 판정
	bool ForceSendPacket = false;

	if (LastDesiredInput != DesiredInput)
	{
		ForceSendPacket = true;
		LastDesiredInput = DesiredInput;
	}

	// State 판정
	if (DesiredInput == FVector2D::Zero())
		SetMoveState(Protocol::MOVE_STATE_IDLE);
	else
		SetMoveState(Protocol::MOVE_STATE_RUN);

	MovePacketSendTimer -= DeltaTime;

	if (MovePacketSendTimer <= 0 || ForceSendPacket)
	{
		MovePacketSendTimer = MOVE_PACKET_SEND_DELAY;

		Protocol::C_MOVE MovePkt;

		// 현재 위치 정보
		{
			Protocol::PosInfo* Info = MovePkt.mutable_info();
			Info->CopyFrom(*SrcInfo);
			Info->set_yaw(DesiredYaw);
			Info->set_state(GetMoveState());
		}

		SEND_PACKET(MovePkt);
	}
}

void AP1MyPlayer::Init(const Protocol::ObjectInfo& ObjectInfo_)
{
    Super::Init(ObjectInfo_);

    SetPosInfo(ObjectInfo_.pos_info());

    _PlayerInfo->CopyFrom(ObjectInfo_.player_info());

    InventoryHelper->Init(_PlayerInfo->mutable_inventory(), this);
    EquippedGearHelper->Init(_PlayerInfo, this);
}

void AP1MyPlayer::SetLevel(int32 Level_)
{
    _PlayerInfo->set_level(Level_);
    OnLevelChanged.Broadcast(_PlayerInfo->level());
}

void AP1MyPlayer::SetExp(int32 CurExp, int32 MaxExp)
{
    _PlayerInfo->set_cur_exp(CurExp);
    if(MaxExp > 0)
        _PlayerInfo->set_max_exp(MaxExp);

    OnExpChanged.Broadcast(_PlayerInfo->cur_exp(), _PlayerInfo->max_exp());
}

void AP1MyPlayer::SetStatInfo(const Protocol::StatInfo& StatInfo_)
{
    _StatInfo->CopyFrom(StatInfo_);
    OnStatInfoChanged.Broadcast(*_StatInfo);
}

void AP1MyPlayer::SetGold(int64 Gold)
{
    _PlayerInfo->set_gold(Gold);
    OnGoldChanged.Broadcast(_PlayerInfo->gold());
}

void AP1MyPlayer::SetInventorySlot(const Protocol::Slot& Slot_, bool OnUse)
{
    InventoryHelper->SetSlot(Slot_);
    OnInventorySlotChanged.Broadcast(Slot_, OnUse);
}

void AP1MyPlayer::SetEquippedGearSlot(const Protocol::Slot& Slot_)
{
    EquippedGearHelper->SetSlot(Slot_);
    OnEquippedGearSlotChanged.Broadcast(Slot_);
}

void AP1MyPlayer::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);

		// Cache
		{
			DesiredInput = MovementVector;

			DesiredMoveDirection = FVector::ZeroVector;
			DesiredMoveDirection += ForwardDirection * MovementVector.Y;
			DesiredMoveDirection += RightDirection * MovementVector.X;
			DesiredMoveDirection.Normalize();

			const FVector Location = GetActorLocation();
			FRotator Rotator = UKismetMathLibrary::FindLookAtRotation(Location, Location + DesiredMoveDirection);
			DesiredYaw = Rotator.Yaw;
		}
	}
}

void AP1MyPlayer::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

