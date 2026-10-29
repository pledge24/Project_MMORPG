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

    // 공격 컴포넌트 연결. 생성자에서 만든 하위 클래스(몬스터)는 그 컴포넌트를 그대로 쓴다.
    // 덮어쓰면 블루프린트가 붙인 다른 공격 컴포넌트가 잡혀, 스폰 때 넣은 몽타주가 재생되지 않을 수 있다.
    if (AttackSystemComponent == nullptr)
        AttackSystemComponent = FindComponentByClass<UP1AttackSystemComponent>();
    if (AttackSystemComponent == nullptr)
        UE_LOG(LogP1CharacterComp, Warning, TEXT("AP1Creature {%s} AttackSystemComponent 누락"), *this->GetName());

    // 네임플레이트에 이 크리처 바인딩.
    // 플레이어는 말풍선 위젯 컴포넌트도 가지므로, 위젯 컴포넌트 중 크리처에 붙는 위젯을 가진 것을 네임플레이트로 고른다.
    NameplateComponent = nullptr;
    TArray<UWidgetComponent*> WidgetComponents;
    GetComponents<UWidgetComponent>(WidgetComponents);
    for (UWidgetComponent* WidgetComponent : WidgetComponents)
    {
        if (IP1CreatureBoundWidget* NameplateWidget = Cast<IP1CreatureBoundWidget>(WidgetComponent->GetWidget()))
        {
            NameplateComponent = WidgetComponent;
            NameplateWidget->BindCreature(this);
            break;
        }
    }

    if (NameplateComponent == nullptr)
        UE_LOG(LogP1CharacterComp, Warning, TEXT("AP1Creature {%s} 네임플레이트 위젯을 가진 위젯 컴포넌트가 없다"), *GetName());

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
    AttackSystemComponent->PlayRemoteNormalAttack(Combo);
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


