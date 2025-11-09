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

bool AMonsterSpawner::SpawnMonster(int32 TemplateId, AMonster* OutMonster, const Protocol::ObjectInfo& InObjectInfo)
{
    const Protocol::PosInfo& PosInfo_ = InObjectInfo.pos_info();

    FVector Location = FVector(PosInfo_.x(), PosInfo_.y(), PosInfo_.z());
    FRotator Rotation = FRotator(0, PosInfo_.yaw(), 0);

    return SpawnMonster(TemplateId, OutMonster, Location, Rotation, InObjectInfo);
}

bool AMonsterSpawner::SpawnMonster(int32 TemplateId, AMonster* OutMonster, const FVector& Location, const FRotator& Rotation, TOptional<Protocol::ObjectInfo> ServerInfo)
{
    if (MonsterDataTable == nullptr)
        return false;

    FMonsterData MonsterData;
    if (GetMonsterData(TemplateId, MonsterData) == false)
        return false;

    TSubclassOf<AMonster> MonsterBPClass = MonsterData.MonsterClass.Get();

    AMonster* TempMonster = GetWorld()->SpawnActorDeferred<AMonster>(
        MonsterBPClass,
        FTransform(Rotation, Location),
        nullptr, 
        nullptr, 
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    if (TempMonster == nullptr)
        return false;

    // 스폰 전에 몬스터 데이터 설정
    {
        if (ServerInfo.IsSet())
            TempMonster->Initialize(ServerInfo.GetValue());

        TempMonster->SetDefaultMonsterData(MonsterData);
        OutMonster = TempMonster;
        OutMonster->FinishSpawning(FTransform(Rotation, Location));
    }

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


