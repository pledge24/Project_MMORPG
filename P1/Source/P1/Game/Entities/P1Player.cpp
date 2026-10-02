#include "Game/Entities/P1Player.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/Equipment/P1GearAppearanceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

AP1Player::AP1Player()
{
	// 아래 WeaponMesh 생성 전까지는 UE의 3인칭 템플릿에서 그대로 가져온 설정이다.
	// 충돌 캡슐의 크기를 정한다
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// 컨트롤러가 회전해도 캐릭터는 돌지 않는다. 컨트롤러 회전은 카메라에만 적용한다.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 캐릭터 이동을 설정한다
	GetCharacterMovement()->bOrientRotationToMovement = true; // 입력 방향으로 몸을 돌린다	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // 몸을 돌리는 속도다

	// 이 값들은 다시 컴파일하지 않고 캐릭터 블루프린트에서 조정할 수 있다
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	//GetCharacterMovement()->bRunPhysicsWithNoController = true;

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(GetMesh(), FName("weapon_r"));

    // 방어구는 캐릭터 메시의 포즈를 그대로 따르는 스켈레탈 메시다. 리더 포즈는 PostInitializeComponents에서 건다.
    HelmetMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HelmetMesh"));
    HelmetMesh->SetupAttachment(GetMesh());
    ChestMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ChestMesh"));
    ChestMesh->SetupAttachment(GetMesh());
    LegsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegsMesh"));
    LegsMesh->SetupAttachment(GetMesh());
    ArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ArmsMesh"));
    ArmsMesh->SetupAttachment(GetMesh());
    BootsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BootsMesh"));
    BootsMesh->SetupAttachment(GetMesh());

    GearAppearance = CreateDefaultSubobject<UP1GearAppearanceComponent>(TEXT("GearAppearance"));
}

void AP1Player::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    for (USkeletalMeshComponent* ArmorMesh : { HelmetMesh.Get(), ChestMesh.Get(), LegsMesh.Get(), ArmsMesh.Get(), BootsMesh.Get() })
    {
        ArmorMesh->SetLeaderPoseComponent(GetMesh());
    }

    // 서버가 보내는 부위 번호(GearType)와 메시 컴포넌트를 잇는다.
    GearAppearance->RegisterGearMesh(Protocol::GEAR_TYPE_HELMET, HelmetMesh);
    GearAppearance->RegisterGearMesh(Protocol::GEAR_TYPE_CHEST, ChestMesh);
    GearAppearance->RegisterGearMesh(Protocol::GEAR_TYPE_LEGS, LegsMesh);
    GearAppearance->RegisterGearMesh(Protocol::GEAR_TYPE_ARMS, ArmsMesh);
    GearAppearance->RegisterGearMesh(Protocol::GEAR_TYPE_BOOTS, BootsMesh);
    GearAppearance->RegisterGearMesh(Protocol::GEAR_TYPE_WEAPON, WeaponMesh);
}

void AP1Player::BeginPlay()
{
	Super::BeginPlay();
}

void AP1Player::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AP1Player::Initialize(const Protocol::EntityInfo& EntityInfo)
{
    Super::Initialize(EntityInfo);

    // 입장할 때 이미 장착 중인 장비를 입힌다. 다른 플레이어도 이 요약으로 외형을 받는다.
    for (const auto& Pair : EntityInfo.player_info().equipped_gear_summary())
    {
        ApplyGear(Pair.first, Pair.second);
    }
}

void AP1Player::ApplyGear(int32 GearType, int32 TemplateId)
{
    GearAppearance->ApplyGear(GearType, TemplateId);
}

void AP1Player::SetPlayerName(const FText& InName)
{
    SetCreatureName(InName);
}
