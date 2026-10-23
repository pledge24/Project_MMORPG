#include "Game/Entities/P1Player.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/Equipment/P1GearAppearanceComponent.h"
#include "Game/Entities/P1ChatBubbleDisplay.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

AP1Player::AP1Player()
{
	// 아래 WeaponMesh 생성 전까지는 UE의 3인칭 템플릿에서 그대로 가져온 설정이다.
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// 컨트롤러가 회전해도 캐릭터는 돌지 않는다. 컨트롤러 회전은 카메라에만 적용한다.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// 아래 값은 캐릭터 블루프린트에서 조정할 수 있다. 블루프린트에서 바꾼 값은 여기를 고쳐도 바뀌지 않는다.
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

    // 말풍선은 화면 공간에 원래 크기로 그리고, 아래 가운데를 기준점으로 삼아 줄이 늘면 위로 자란다.
    // 그래서 기준점을 네임플레이트 위에 두면 몇 줄이 되든 네임플레이트를 덮지 않는다.
    ChatBubbleComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ChatBubble"));
    ChatBubbleComponent->SetupAttachment(RootComponent);
    ChatBubbleComponent->SetWidgetSpace(EWidgetSpace::Screen);
    ChatBubbleComponent->SetDrawAtDesiredSize(true);
    ChatBubbleComponent->SetPivot(FVector2D(0.5f, 1.f));
    ChatBubbleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ChatBubbleComponent->SetGenerateOverlapEvents(false);
}

void AP1Player::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    for (USkeletalMeshComponent* ArmorMesh : { HelmetMesh.Get(), ChestMesh.Get(), LegsMesh.Get(), ArmsMesh.Get(), BootsMesh.Get() })
    {
        ArmorMesh->SetLeaderPoseComponent(GetMesh());
    }

    // 서버가 보내는 부위 번호(GearPart)와 메시 컴포넌트를 잇는다.
    GearAppearance->RegisterGearMesh(Protocol::GEAR_PART_HELMET, HelmetMesh);
    GearAppearance->RegisterGearMesh(Protocol::GEAR_PART_CHEST, ChestMesh);
    GearAppearance->RegisterGearMesh(Protocol::GEAR_PART_LEGS, LegsMesh);
    GearAppearance->RegisterGearMesh(Protocol::GEAR_PART_ARMS, ArmsMesh);
    GearAppearance->RegisterGearMesh(Protocol::GEAR_PART_BOOTS, BootsMesh);
    GearAppearance->RegisterGearMesh(Protocol::GEAR_PART_WEAPON, WeaponMesh);
}

void AP1Player::BeginPlay()
{
	Super::BeginPlay();

    // 말풍선은 말하기 전까지 숨긴다.
    if (IP1ChatBubbleDisplay* Bubble = GetChatBubble())
        Bubble->SetBubbleVisible(false);
}

void AP1Player::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(ChatBubbleHideTimer);
    GetWorldTimerManager().ClearTimer(ChatBubbleRangeTimer);

    Super::EndPlay(EndPlayReason);
}

void AP1Player::Initialize(const Protocol::EntityInfo& EntityInfo)
{
    Super::Initialize(EntityInfo);

    // 입장할 때 이미 장착 중인 장비를 입힌다. 다른 플레이어도 이 요약으로 외형을 받는다.
    for (const auto& Pair : EntityInfo.player_info().gear_appearance())
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

void AP1Player::ShowChatBubble(const FString& Message)
{
    IP1ChatBubbleDisplay* Bubble = GetChatBubble();
    if (Bubble == nullptr)
        return;

    Bubble->SetBubbleMessage(FText::FromString(Message));
    bChatBubbleActive = true;
    RefreshChatBubbleVisibility();

    // 연달아 말하면 앞 말풍선을 새 말로 바꾸고 표시 시간을 처음부터 다시 센다.
    FTimerManager& TimerManager = GetWorldTimerManager();
    TimerManager.SetTimer(ChatBubbleHideTimer, this, &AP1Player::HideChatBubble, CHAT_BUBBLE_SECONDS, false);
    TimerManager.SetTimer(ChatBubbleRangeTimer, this, &AP1Player::RefreshChatBubbleVisibility, CHAT_BUBBLE_RANGE_CHECK_SECONDS, true);
}

bool AP1Player::IsWithinChatBubbleRange(const FVector& SpeakerLocation, const FVector& ViewerLocation, float MaxDistance)
{
    return FVector::DistSquared(SpeakerLocation, ViewerLocation) <= FMath::Square(MaxDistance);
}

void AP1Player::HideChatBubble()
{
    bChatBubbleActive = false;
    GetWorldTimerManager().ClearTimer(ChatBubbleRangeTimer);

    if (IP1ChatBubbleDisplay* Bubble = GetChatBubble())
        Bubble->SetBubbleVisible(false);
}

void AP1Player::RefreshChatBubbleVisibility()
{
    IP1ChatBubbleDisplay* Bubble = GetChatBubble();
    if (Bubble == nullptr)
        return;

    // 보는 쪽은 이 클라이언트의 내 플레이어다. PIE 창마다 월드가 따로라 그 월드의 첫 컨트롤러가 그 창의 플레이어다.
    // 내 플레이어가 아직 없으면 거리를 잴 수 없으니 보인다.
    const UWorld* World = GetWorld();
    const APlayerController* LocalController = World ? World->GetFirstPlayerController() : nullptr;
    const APawn* Viewer = LocalController ? LocalController->GetPawn() : nullptr;
    const bool bInRange = Viewer == nullptr || IsWithinChatBubbleRange(GetActorLocation(), Viewer->GetActorLocation(), ChatBubbleVisibleDistance);

    Bubble->SetBubbleVisible(bChatBubbleActive && bInRange);
}

IP1ChatBubbleDisplay* AP1Player::GetChatBubble() const
{
    return ChatBubbleComponent ? Cast<IP1ChatBubbleDisplay>(ChatBubbleComponent->GetWidget()) : nullptr;
}
