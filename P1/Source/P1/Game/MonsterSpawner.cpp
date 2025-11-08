// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/MonsterSpawner.h"
#include "Monster.h"

AMonsterSpawner::AMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = true;

}

bool AMonsterSpawner::SpawnMonster(int32 TemplateId, AMonster* OutMonster, const FTransform& Transform)
{
    FVector Location = Transform.GetLocation();
    FRotator Rotation = Transform.GetRotation().Rotator();

    return SpawnMonster(TemplateId, OutMonster, Location, Rotation);
}

bool AMonsterSpawner::SpawnMonster(int32 TemplateId, AMonster* OutMonster, const FVector& Location, const FRotator& Rotation)
{
    FMonsterData MonsterData;
    if (GetMonsterData(TemplateId, MonsterData) == false)
        return false;

    // Spawn And Set Default Monster
    TSubclassOf<AMonster> MonsterBPClass = MonsterData.MonsterClass.Get();
    /*OutMonster = GetWorld()->SpawnActor<AMonster>(MonsterBPClass, Location, Rotation);
    OutMonster->SetDefaultMonsterData(MonsterData);*/

    AMonster* TempMonster = GetWorld()->SpawnActorDeferred<AMonster>(
        MonsterBPClass,
        FTransform(Rotation, Location),
        nullptr, 
        nullptr, 
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    if (TempMonster)
    {
        // 예를 들어, 몬스터의 특정 데이터를 여기서 설정할 수 있습니다.
        TempMonster->SetDefaultMonsterData(MonsterData);

        // 최종적으로 액터 포인터를 OutMonster 변수에 할당합니다.
        OutMonster = TempMonster;

        // 4. FinishSpawningActor를 호출하여 최종 초기화(BeginPlay 등)를 완료합니다.
        OutMonster->FinishSpawning(FTransform(Rotation, Location));
    }

    return true;
}

bool AMonsterSpawner::SpawnMonster(int32 TemplateId, AMonster* OutMonster, const Protocol::ObjectInfo& InObjectInfo)
{
    const Protocol::PosInfo& PosInfo_ = InObjectInfo.pos_info();

    FVector Location = FVector(PosInfo_.x(), PosInfo_.y(), PosInfo_.z());
    FRotator Rotation = FRotator(0, PosInfo_.yaw(), 0);

    FMonsterData MonsterData;
    if (GetMonsterData(TemplateId, MonsterData) == false)
        return false;

    // Spawn And Set Default Monster
    TSubclassOf<AMonster> MonsterBPClass = MonsterData.MonsterClass.Get();
    OutMonster = GetWorld()->SpawnActor<AMonster>(MonsterBPClass, Location, Rotation);
    OutMonster->Initialize(InObjectInfo);
    OutMonster->SetDefaultMonsterData(MonsterData);

    return true;
}

//void AMonsterSpawner::LoadMonsterAsset()
//{
//    TArray<FMonsterData*> AllRows;
//    static const FString Context(TEXT("LoadMonsterAsset"));
//
//    MonsterDataTable->GetAllRows<FMonsterData>(Context, AllRows);
//    for (const FMonsterData* Row : AllRows)
//    {
//        if (!Row)
//            continue;
//
//        Row->MonsterClass.LoadSynchronous();
//
//    }
//}

bool AMonsterSpawner::GetMonsterData(int32 TemplateId, FMonsterData& OutMonsterData)
{
    FName RowName = *FString::FromInt(TemplateId);

    if (MonsterDataTable == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("MonsterDataTable Is Null"));
        return false;
    }

    FMonsterData* FoundRow = MonsterDataTable->FindRow<FMonsterData>(RowName, TEXT("GetRowData"), true);

    if (!FoundRow)
    {
        UE_LOG(LogTemp, Warning, TEXT("RowName{%s} Is Not Exist"), *RowName.ToString());
        return false;
    }

    if (!FoundRow->MonsterClass)
        FoundRow->MonsterClass.LoadSynchronous();

    OutMonsterData = *FoundRow;

    return true;
}

void AMonsterSpawner::BeginPlay()
{
	Super::BeginPlay();

    if (MonsterDataTable == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("AMonsterSpawner에서 MonsterDataTable 누락"));
        return;
    }

}


