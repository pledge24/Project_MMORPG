// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster.h"

#include "Components/WidgetComponent.h"
#include "Widgets/NameplateWidget.h"

AMonster::AMonster()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AMonster::BeginPlay()
{
	Super::BeginPlay();

}

void AMonster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

}

void AMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMonster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AMonster::Initialize(const Protocol::ObjectInfo& ObjectInfo)
{
    Super::Initialize(ObjectInfo);

    if (ObjectInfo.has_monster_info() == false)
    {
        UE_LOG(LogTemp, Warning, TEXT("몬스터 정보가 없는채로 몬스터 초기화 시도함"));
        return;
    }

    _MonsterInfo.CopyFrom(ObjectInfo.monster_info());

    TemplateId = _MonsterInfo.template_id();
    CurHp = _MonsterInfo.hp();
}

void AMonster::SetDefaultMonsterData(const FMonsterData& InMonsterData)
{
    MonsterData = InMonsterData;

    CreatureName = FText::FromString(MonsterData.MonsterName);
    TemplateId = MonsterData.TemplateId;
    if (CurHp < 0)
        CurHp = MonsterData.MaxHp;

    UE_LOG(LogTemp, Log, TEXT("SetDefaultMonsterData"));
}

