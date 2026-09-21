#include "Characters/P1Monster.h"

#include "Components/WidgetComponent.h"
#include "UI/P1NameplateWidget.h"
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

void AP1Monster::SetDefaultMonsterData(const FP1MonsterData& InMonsterData)
{
    MonsterData = InMonsterData;

    CreatureName = FText::FromString(MonsterData.MonsterName);
    TemplateId = MonsterData.TemplateId;
    if (CurHp < 0)
        CurHp = MonsterData.MaxHp;
}

