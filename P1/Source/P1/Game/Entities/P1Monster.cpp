#include "Game/Entities/P1Monster.h"

#include "Components/WidgetComponent.h"
#include "Sync/P1StatefulEntityManager.h"
#include "Utils/LogCategory.h"

AP1Monster::AP1Monster()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AP1Monster::BeginPlay()
{
	Super::BeginPlay();

}

void AP1Monster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

}

void AP1Monster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AP1Monster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AP1Monster::Initialize(const Protocol::EntityInfo& EntityInfo)
{
    Super::Initialize(EntityInfo);

    if (EntityInfo.has_monster_info() == false)
    {
        UE_LOG(LogP1CharacterComp, Warning, TEXT("몬스터 정보가 없는채로 몬스터 초기화 시도함"));
        return;
    }

    _MonsterInfo.CopyFrom(EntityInfo.monster_info());

    TemplateId = _MonsterInfo.template_id();
    CurHp = _MonsterInfo.hp();
}

void AP1Monster::S_Die()
{
    Super::S_Die();

    // 사망 애니메이션은 ABP가 OnDie를 받아 재생한다. 그 시간을 준 뒤 지운다.
    // 그사이 룸 이동으로 먼저 지워지면 AActor::EndPlay가 이 타이머를 함께 지운다.
    GetWorldTimerManager().SetTimer(DespawnTimerHandle, this, &AP1Monster::DespawnAfterDeath, DESPAWN_DELAY_SECONDS, false);
}

void AP1Monster::DespawnAfterDeath()
{
    // Destroy만 하면 매니저의 등록이 남는다. 매니저를 거쳐 등록과 액터를 함께 지운다.
    if (UP1StatefulEntityManager* StatefulEntityManager = GetWorld()->GetSubsystem<UP1StatefulEntityManager>())
    {
        StatefulEntityManager->DespawnEntity(GetPosInfo()->entity_id());
    }
}

void AP1Monster::SetDefaultMonsterData(const FP1MonsterData& InMonsterData)
{
    MonsterData = InMonsterData;

    CreatureName = FText::FromString(MonsterData.MonsterName);
    TemplateId = MonsterData.TemplateId;
    if (CurHp < 0)
        CurHp = MonsterData.MaxHp;
}

