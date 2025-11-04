// Fill out your copyright notice in the Description page of Project Settings.


#include "P1MyPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "P1.h"
#include "AttackSystemComponent.h"
#include "Inventory.h"
#include "EquippedGear.h"
#include "Log/LogCategory.h"

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

    // Note: The skeletal mesh and anim blueprint references on the CharacterMesh component (inherited from Character) 
    // are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AP1MyPlayer::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        PC->Possess(this);

	    //Add Input Mapping Context
	    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	    {
		    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		    {
			    Subsystem->AddMappingContext(DefaultMappingContext, 0);
		    }
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
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AP1MyPlayer::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AP1MyPlayer::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AP1MyPlayer::Look);

        //Attacking
        EnhancedInputComponent->BindAction(NormalAttackAction, ETriggerEvent::Started, this, &AP1MyPlayer::NormalAttack);
	}

}

void AP1MyPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    bool bForceSendPacket = false; // MovePacket 강제 전송 판정 변수
    bool bCanInputMovement = CanInputMovement();

    // bForceSendPacket 판정
	{
        // 입력 변화 감지
	    if (LastDesiredInput != DesiredInput)
	    {
            if (bCanInputMovement)
		        bForceSendPacket = true;
            LastDesiredInput = DesiredInput;
	    }

        // 급격한 회전 감지
        FRotator DeltaRotator = FRotator(0.f, DesiredYaw, 0.f) - FRotator(0.f, MovePkt.info().desired_yaw(), 0.f);
        if (FMath::Abs(DeltaRotator.Yaw) >= YAW_TOLERANCE)
        {
            bForceSendPacket = true;
        }
	}

	// State 판정
    if (AttackSystemComponent->IsAttacking() == true)
    {
        SetMoveState(Protocol::MOVE_STATE_ACTION);
        bForceSendPacket = false;
    }
    else if (bCanInputMovement && DesiredInput != FVector2D::Zero())
		SetMoveState(Protocol::MOVE_STATE_RUN);
    else
		SetMoveState(Protocol::MOVE_STATE_IDLE);

    // Send 판정
    MovePacketSendTimer -= DeltaTime;

    if (MovePacketSendTimer <= 0 || bForceSendPacket)
    {
        /*SendCounter++;
        TotalSecond += MOVE_PACKET_SEND_DELAY - MovePacketSendTimer;
        GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Red, FString::Printf(TEXT("AvgSendSpeed: %f"), TotalSecond / SendCounter));*/

        MovePacketSendTimer = MOVE_PACKET_SEND_DELAY;

        // 현재 위치 정보
        {
            MovePkt.Clear();

            Protocol::PosInfo* Info = MovePkt.mutable_info();
            Info->CopyFrom(*ClientPos);
            Info->set_desired_yaw(DesiredYaw);
            Info->set_state(GetMoveState());
        }

        SEND_PACKET(MovePkt);
    }
}

void AP1MyPlayer::Initialize(const Protocol::ObjectInfo& InObjectInfo)
{
    Super::Initialize(InObjectInfo);

    SetClientPos(InObjectInfo.pos_info());
}

void AP1MyPlayer::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

    UE_LOG(LogTemp, Log, TEXT("MOVE: (%f, %f)"), MovementVector.Y, MovementVector.X);
	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        if (CanInputMovement() == true)
        {
		    // add movement 
		    AddMovementInput(ForwardDirection, MovementVector.Y);
		    AddMovementInput(RightDirection, MovementVector.X);
        }

		// Cache Movement Input
		{
			DesiredInput = MovementVector;

			DesiredMoveDirection = FVector::ZeroVector;
			DesiredMoveDirection += ForwardDirection * MovementVector.Y;
			DesiredMoveDirection += RightDirection * MovementVector.X;
			DesiredMoveDirection.Normalize();

            DesiredYaw = DesiredMoveDirection.Rotation().Yaw;
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

void AP1MyPlayer::NormalAttack(const FInputActionValue& Value)
{
    UStaticMesh* StaticMesh = WeaponMesh->GetStaticMesh();
    if (!StaticMesh)
    {
        UE_LOG(LogCharacterComp, Display, TEXT("무기없이 일반 공격을 수행할 수 없습니다."));
        return;
    }

    if (AttackSystemComponent != nullptr)
    {
        if (AttackSystemComponent->EnableInputAttack() == true)
        {
            AttackSystemComponent->PerformNormalAttack();
            int32 Combo = AttackSystemComponent->GetLastCombo();

            if (Combo > 0){
                Protocol::C_NORMAL_ATTACK NormalAttackPkt;
                NormalAttackPkt.set_combo(Combo);

                SEND_PACKET(NormalAttackPkt);
            }
        }
    }
}

bool AP1MyPlayer::CanInputMovement() const
{
    if (AttackSystemComponent != nullptr && AttackSystemComponent->IsAttacking() == true)
        return false;

    return true;
}

