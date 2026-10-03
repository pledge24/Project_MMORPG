#include "Game/Entities/P1Creature.h"
#include "Game/Combat/P1AttackSystemComponent.h"
#include "Game/Entities/P1CreatureBoundWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Utils/LogCategory.h"

AP1Creature::AP1Creature()
{
	PrimaryActorTick.bCanEverTick = true;

    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    GetCharacterMovement()->bRunPhysicsWithNoController = true;
}

void AP1Creature::BeginPlay()
{
    Super::BeginPlay();

    // 공격 컴포넌트를 찾는다
    AttackSystemComponent = FindComponentByClass<UP1AttackSystemComponent>();
    if (AttackSystemComponent == nullptr)
        UE_LOG(LogP1CharacterComp, Warning, TEXT("AP1Creature {%s} AttackSystemComponent 누락"), *this->GetName());

    // 네임플레이트 컴포넌트를 찾아 이 크리처를 바인딩한다
    NameplateComponent = FindComponentByClass<UWidgetComponent>();
    if (NameplateComponent)
    {
        if (IP1CreatureBoundWidget* NameplateWidget = Cast<IP1CreatureBoundWidget>(NameplateComponent->GetWidget()))
            NameplateWidget->BindCreature(this);
        else
            UE_LOG(LogP1CharacterComp, Warning, TEXT("AP1Creature::BeginPlay() NameplateWidget 누락"));
    }
    else
    {
        UE_LOG(LogP1CharacterComp, Warning, TEXT("AP1Creature::BeginPlay() NameplateComponent 누락"));
    }

}

void AP1Creature::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AP1Creature::Initialize(const Protocol::EntityInfo& EntityInfo)
{
    FText InName = FText::FromString(UTF8_TO_TCHAR(EntityInfo.player_info().name().c_str()));
    SetCreatureName(InName);
}

bool AP1Creature::IsAttacking() const
{
    return AttackSystemComponent != nullptr && AttackSystemComponent->IsAttacking();
}

void AP1Creature::SetCreatureName(const FText& InName)
{
    CreatureName = InName;
}

void AP1Creature::SetDeadState(bool IsDead)
{
    _IsDead = IsDead;

    if (IsDead)
    {
        GetCharacterMovement()->DisableMovement();
        GetCharacterMovement()->StopMovementImmediately();
    }
    else
    {
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
}

void AP1Creature::S_NormalAttack(uint32 Combo, float Yaw)
{
    if (IsValid(AttackSystemComponent) == false)
        return;

    SetActorRotation(FRotator(0, Yaw, 0));
    AttackSystemComponent->PlayNotifiedNormalAttack(Combo);
}

void AP1Creature::S_Hit(int64 Damage, int64 UpdatedHp)
{
    OnHit.Broadcast(Damage, UpdatedHp);
}

void AP1Creature::S_Die()
{
    SetDeadState(true);

    OnDie.Broadcast(this);
}

void AP1Creature::S_Respawn()
{
    SetDeadState(false);

    OnRespawn.Broadcast(this);
}


