#include "Game/Entities/P1MyPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Network/P1PacketSender.h"
#include "Game/Combat/P1AttackSystemComponent.h"
#include "Utils/LogCategory.h"

AP1MyPlayer::AP1MyPlayer()
{
    // 카메라 붐을 만든다. 벽에 막히면 플레이어 쪽으로 당겨진다
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f; // 카메라가 캐릭터 뒤에서 따라오는 거리다	
    CameraBoom->bUsePawnControlRotation = true; // 암은 컨트롤러 회전을 따른다

    // 따라가는 카메라를 만든다
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // 붐 끝에 붙여 붐이 컨트롤러 방향에 맞춰 돌게 한다
    FollowCamera->bUsePawnControlRotation = false; // 카메라는 암에 대해 따로 돌지 않는다

    // 엔진이 FinishSpawning 안(PreInitializeComponents)에서 0번 컨트롤러에 빙의시킨다. BeginPlay에서는 Controller가 채워져 있다.
    AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AP1MyPlayer::BeginPlay()
{
	Super::BeginPlay();

    // 입력 매핑 컨텍스트를 붙인다
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    // 스폰 알림(OnMyPlayerSpawned)은 스포너가 Initialize를 마친 뒤에 보낸다.
}

void AP1MyPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

//~ Input

void AP1MyPlayer::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// 입력 액션을 바인딩한다
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 점프
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// 이동
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AP1MyPlayer::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AP1MyPlayer::Move);

		// 시점 회전
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AP1MyPlayer::Look);

        // 공격
        EnhancedInputComponent->BindAction(NormalAttackAction, ETriggerEvent::Started, this, &AP1MyPlayer::NormalAttack);

        // 전투 모드 전환
        EnhancedInputComponent->BindAction(ToggleBattleModeAction, ETriggerEvent::Started, this, &AP1MyPlayer::ToggleBattleMode);

	}

}

void AP1MyPlayer::Initialize(const Protocol::EntityInfo& InEntityInfo)
{
    Super::Initialize(InEntityInfo);
}

void AP1MyPlayer::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// 카메라 기준의 앞 방향을 구한다
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        if (CanInputMovement() == true)
        {
		    // 이동 입력을 넣는다
		    AddMovementInput(ForwardDirection, MovementVector.Y);
		    AddMovementInput(RightDirection, MovementVector.X);
        }

		// 이동 입력을 캐시한다
		{
			DesiredInput = MovementVector;

			DesiredMoveDirectionVec = FVector::ZeroVector;
			DesiredMoveDirectionVec += ForwardDirection * MovementVector.Y;
			DesiredMoveDirectionVec += RightDirection * MovementVector.X;
			DesiredMoveDirectionVec.Normalize();

            DesiredMoveDirectionYaw = DesiredMoveDirectionVec.Rotation().Yaw;
		}
	}
}

void AP1MyPlayer::Look(const FInputActionValue& Value)
{
	// 입력은 2D 벡터다
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// 컨트롤러에 Yaw와 Pitch 입력을 넣는다
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AP1MyPlayer::NormalAttack(const FInputActionValue& Value)
{
    UStaticMesh* StaticMesh = WeaponMesh->GetStaticMesh();
    if (!StaticMesh)
    {
        UE_LOG(LogP1CharacterComp, Display, TEXT("무기없이 일반 공격을 수행할 수 없습니다."));
        return;
    }

    if (AttackSystemComponent != nullptr && bBattleMode == true)
    {
        // 입력을 받을 수 없으면 0이 돌아온다.
        const int32 Combo = AttackSystemComponent->StartNormalAttack();
        if (Combo > 0)
        {
            Protocol::C_NORMAL_ATTACK NormalAttackPkt;
            NormalAttackPkt.set_combo(Combo);
            // 서버의 yaw는 마지막 이동 패킷의 값이라 늦을 수 있다. 다른 클라이언트는 이 값으로 돌려 세운다.
            NormalAttackPkt.set_yaw(GetActorRotation().Yaw);

            FP1PacketSender::Send(this, NormalAttackPkt);
        }
    }
}

void AP1MyPlayer::ToggleBattleMode(const FInputActionValue& Value)
{
    bBattleMode = !bBattleMode;

    // 화면 표시는 컨트롤러가 이 알림을 구독해서 맡는다.
    OnBattleModeChanged.Broadcast(bBattleMode);
}

bool AP1MyPlayer::CanInputMovement() const
{
    return IsAttacking() == false;
}

