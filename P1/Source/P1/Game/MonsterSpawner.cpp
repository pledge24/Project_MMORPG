// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/MonsterSpawner.h"

AMonsterSpawner::AMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = true;

}

AMonster* AMonsterSpawner::SpawnMonster(int32 TemplateId, const FTransform& Transform)
{
    FVector Location = Transform.GetLocation();
    FRotator Rotation = Transform.GetRotation().Rotator();

    return SpawnMonster(TemplateId, Location, Rotation);
}

AMonster* AMonsterSpawner::SpawnMonster(int32 TemplateId, const FVector& Location, const FRotator& Rotation)
{
    FMonsterData MonsterData;
    if (GetMonsterData(TemplateId, MonsterData) == false)
        return nullptr;

    TSubclassOf<AMonster> MonsterBPClass = MonsterData.MonsterClass.Get();
    AMonster* Monster = GetWorld()->SpawnActor<AMonster>(MonsterBPClass, Location, Rotation);

    return Monster;
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


