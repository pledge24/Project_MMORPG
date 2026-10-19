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
    // 카메라 붐은 벽에 막히면 플레이어 쪽으로 당겨진다. 카메라는 붐 끝에 붙어 붐과 함께 컨트롤러 방향으로 돈다.
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // 엔진이 FinishSpawning 안(PreInitializeComponents)에서 0번 컨트롤러에 빙의시킨다. BeginPlay에서는 Controller가 채워져 있다.
    AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AP1MyPlayer::BeginPlay()
{
	Super::BeginPlay();

    // 입력 매핑 컨텍스트 등록.
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    // 스폰 알림(OnMyPlayerSpawned)은 여기서 보내지 않는다. 스포너가 Initialize를 마친 뒤에 보내야
    // 구독자가 서버 정보로 초기화된 플레이어를 받는다.
}

void AP1MyPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AP1MyPlayer::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// 입력 액션 바인딩.
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// 입력을 놓을 때(Completed)도 Move를 불러 캐시한 이동 입력을 0으로 되돌린다.
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AP1MyPlayer::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AP1MyPlayer::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AP1MyPlayer::Look);
        EnhancedInputComponent->BindAction(NormalAttackAction, ETriggerEvent::Started, this, &AP1MyPlayer::NormalAttack);
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
		// 이동 방향은 카메라(컨트롤러 회전)의 Yaw 기준이다.
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        if (CanInputMovement() == true)
        {
		    AddMovementInput(ForwardDirection, MovementVector.Y);
		    AddMovementInput(RightDirection, MovementVector.X);
        }

		// 공격 중이라 이동하지 못해도 입력은 캐시한다. 이동 동기화가 이 값과 CanInputMovement로
		// 이동 상태와 송신을 판정한다.
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
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
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

