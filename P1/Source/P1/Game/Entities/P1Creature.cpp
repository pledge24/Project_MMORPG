#include "Game/Entities/P1Creature.h"
#include "Game/Combat/P1ActionComponent.h"
#include "Game/Entities/P1CreatureBoundWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Utils/LogCategory.h"

AP1Creature::AP1Creature()
{
	PrimaryActorTick.bCanEverTick = true;

    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    GetCharacterMovement()->bRunPhysicsWithNoController = true;

    // 액션은 액션 에셋 표에서 몽타주를 찾으므로 크리처마다 몽타주를 넣을 블루프린트 컴포넌트가 없다.
    ActionComponent = CreateDefaultSubobject<UP1ActionComponent>(TEXT("Action"));
}

void AP1Creature::BeginPlay()
{
    Super::BeginPlay();

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
    return ActionComponent != nullptr && ActionComponent->IsActing();
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

void AP1Creature::S_ActionStart(int32 ActionId, float Yaw)
{
    if (IsValid(ActionComponent) == false)
        return;

    // 서버는 액션을 시작한 방향으로 판정하고, 액션이 끝날 때까지 방향을 바꾸지 않는다.
    SetActorRotation(FRotator(0, Yaw, 0));
    ActionComponent->PlayRemoteAction(ActionId);
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


