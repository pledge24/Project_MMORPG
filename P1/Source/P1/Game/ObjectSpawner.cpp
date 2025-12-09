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

AActor* AObjectSpawner::SpawnMonster(int32 TemplateId, const FTransform& Transform)
{
    FVector Location = Transform.GetLocation();
    FRotator Rotation = Transform.GetRotation().Rotator();

    return SpawnMonster(TemplateId, Location, Rotation);
}

AActor* AObjectSpawner::SpawnMonster(const Protocol::ObjectInfo& InObjectInfo)
{
    int32 TemplateId = InObjectInfo.monster_info().template_id();
    const Protocol::PosInfo& PosInfo_ = InObjectInfo.pos_info();
    const Protocol::Vector& Pos = PosInfo_.pos();

    FVector Location = FVector(Pos.x(), Pos.y(), Pos.z());
    FRotator Rotation = FRotator(0, PosInfo_.yaw(), 0);

    return SpawnMonster(TemplateId, Location, Rotation, InObjectInfo);
}

AActor* AObjectSpawner::SpawnMonster(int32 TemplateId, const FVector& SpawnLocation, const FRotator& SpawnRotation, TOptional<Protocol::ObjectInfo> ServerInfo)
{
    if (MonsterDataTable == nullptr)
        return nullptr;

    FMonsterData MonsterData;
    if (GetMonsterData(TemplateId, MonsterData) == false)
        return nullptr;

    TSubclassOf<AMonster> MonsterBPClass = MonsterData.MonsterClass.Get();

    AMonster* OutMonster = GetWorld()->SpawnActorDeferred<AMonster>(
        MonsterBPClass,
        FTransform(SpawnRotation, SpawnLocation),
        nullptr, 
        nullptr, 
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    // 스폰 전에 몬스터 데이터 설정
    if (OutMonster != nullptr)
    {
        if (ServerInfo.IsSet())
        {
            OutMonster->Initialize(ServerInfo.GetValue());
        }

        OutMonster->SetDefaultMonsterData(MonsterData);
        OutMonster->FinishSpawning(FTransform(SpawnRotation, SpawnLocation));
    }

    return OutMonster;
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

AActor* AObjectSpawner::SpawnPlayer(const Protocol::ObjectInfo& InObjectInfo)
{
    UP1GameInstance* GameInstance = Cast<UP1GameInstance>(GetGameInstance());
    if (GameInstance == nullptr)
        return nullptr;

    UWorld* World = GetWorld();
    UMyPlayerData* MyPlayerData = GameInstance->GetMyPlayerData();
    uint64 MyPlayerId = MyPlayerData->GetPlayerId();
    bool IsMine = MyPlayerId == InObjectInfo.object_id();

    FVector SpawnLocation(InObjectInfo.pos_info().pos().x(), InObjectInfo.pos_info().pos().y(), InObjectInfo.pos_info().pos().z());
    FRotator SpawnRotation(0.f, InObjectInfo.pos_info().yaw(), 0.f);

    AP1Player* OutPlayer = nullptr;
    if (IsMine)
    {
        if (!MyPlayerClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("MyPlayerClass가 설정되지 않았습니다"));
            return nullptr;
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
            return nullptr;
        }

        OutPlayer = World->SpawnActorDeferred<AP1Player>(
            OtherPlayerClass,
            FTransform(SpawnRotation, SpawnLocation),
            nullptr,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn
        );

    }

    if (OutPlayer != nullptr)
    {
        // 플레이어 데이터 설정(스폰 전 후로)
        FString PlayerName = InObjectInfo.player_info().name().c_str();
        OutPlayer->SetPlayerName(FText::FromString(PlayerName));
        OutPlayer->SetServerPos(InObjectInfo.pos_info());
        OutPlayer->FinishSpawning(FTransform(SpawnRotation, SpawnLocation));
        OutPlayer->Initialize(InObjectInfo);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("플레이어 SpawnActorDeferred<> NullPtr 반환"));
    }

    return OutPlayer;
}


