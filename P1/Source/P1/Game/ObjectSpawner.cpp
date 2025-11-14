// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/ObjectSpawner.h"
#include "Monster.h"
#include "MyPlayerData.h"
#include "P1GameInstance.h"
#include "StatefulObjectManager.h"

AObjectSpawner::AObjectSpawner()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AObjectSpawner::BeginPlay()
{
    Super::BeginPlay();

    if (MonsterDataTable == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("AMonsterSpawner에서 MonsterDataTable 누락"));
        return;
    }

    if (UStatefulObjectManager* ObjectManager = GetWorld()->GetSubsystem<UStatefulObjectManager>())
    {
        ObjectManager->RegisterSpawner(this);
    }

}

bool AObjectSpawner::SpawnMonster(int32 TemplateId, AActor* OutMonster, const FTransform& Transform)
{
    FVector Location = Transform.GetLocation();
    FRotator Rotation = Transform.GetRotation().Rotator();

    return SpawnMonster(TemplateId, (AMonster*)OutMonster, Location, Rotation);
}

bool AObjectSpawner::SpawnMonster(AMonster* OutMonster, const Protocol::ObjectInfo& InObjectInfo)
{
    int32 TemplateId = InObjectInfo.monster_info().template_id();
    const Protocol::PosInfo& PosInfo_ = InObjectInfo.pos_info();

    FVector Location = FVector(PosInfo_.x(), PosInfo_.y(), PosInfo_.z());
    FRotator Rotation = FRotator(0, PosInfo_.yaw(), 0);

    return SpawnMonster(TemplateId, OutMonster, Location, Rotation, InObjectInfo);
}

bool AObjectSpawner::SpawnMonster(int32 TemplateId, AMonster* OutMonster, const FVector& SpawnLocation, const FRotator& SpawnRotation, TOptional<Protocol::ObjectInfo> ServerInfo)
{
    if (MonsterDataTable == nullptr)
        return false;

    FMonsterData MonsterData;
    if (GetMonsterData(TemplateId, MonsterData) == false)
        return false;

    TSubclassOf<AMonster> MonsterBPClass = MonsterData.MonsterClass.Get();

    AMonster* TempMonster = GetWorld()->SpawnActorDeferred<AMonster>(
        MonsterBPClass,
        FTransform(SpawnRotation, SpawnLocation),
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
        OutMonster->FinishSpawning(FTransform(SpawnRotation, SpawnLocation));
    }

    return true;
}

bool AObjectSpawner::GetMonsterData(int32 TemplateId, FMonsterData& OutMonsterData)
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

bool AObjectSpawner::SpawnPlayer(AP1Player* OutPlayer, const Protocol::ObjectInfo& InObjectInfo)
{
    UP1GameInstance* GameInstance = Cast<UP1GameInstance>(GetGameInstance());
    if (GameInstance == nullptr)
        return false;

    UWorld* World = GetWorld();
    UMyPlayerData* MyPlayerData = GameInstance->GetMyPlayerData();
    uint64 MyPlayerId = MyPlayerData->GetPlayerId();
    bool IsMine = MyPlayerId == InObjectInfo.object_id();

    FVector SpawnLocation(InObjectInfo.pos_info().x(), InObjectInfo.pos_info().y(), InObjectInfo.pos_info().z());
    FRotator SpawnRotation(0.f, InObjectInfo.pos_info().yaw(), 0.f);

    if (IsMine)
    {
        if (!MyPlayerClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("MyPlayerClass가 설정되지 않았습니다"));
            return false;
        }

        OutPlayer = World->SpawnActorDeferred<AP1Player>(
            MyPlayerClass,
            FTransform(SpawnRotation, SpawnLocation),
            nullptr,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn
        );

        // 내 플레이어 Pawn은 GameInstance가 알고 있게한다.
        GameInstance->SetMyPlayer(Cast<AP1MyPlayer>(OutPlayer));
    }
    else
    {
        if (!OtherPlayerClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("OtherPlayerClass가 설정되지 않았습니다"));
            return false;
        }

        OutPlayer = World->SpawnActorDeferred<AP1Player>(
            OtherPlayerClass,
            FTransform(SpawnRotation, SpawnLocation),
            nullptr,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn
        );

    }

    if (OutPlayer == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("플레이어 SpawnActorDeferred<> NullPtr 반환"));
        return false;
    }

    // 플레이어 데이터 설정(스폰 전 후로)
    {
        FString PlayerName = InObjectInfo.player_info().name().c_str();
        OutPlayer->SetPlayerName(FText::FromString(PlayerName));
        OutPlayer->FinishSpawning(FTransform(SpawnRotation, SpawnLocation));
        OutPlayer->Initialize(InObjectInfo);
    }

    UE_LOG(LogTemp, Warning, TEXT("스폰 완료!"));


    return true;
}


