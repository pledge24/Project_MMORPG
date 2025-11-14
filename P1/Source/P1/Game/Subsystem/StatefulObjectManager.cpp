// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Subsystem/StatefulObjectManager.h"

#include "MyPlayerData.h"
#include "ObjectSpawner.h"
#include "P1Player.h"

void UStatefulObjectManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // BP로 설정한 property가 없음
    //UWorld* World = GetWorld();
    //if (AObjectSpawner* ObjectSpawner = World->SpawnActor<AObjectSpawner>())
    //{
    //    RegisterSpawner(ObjectSpawner);
    //}
}

void UStatefulObjectManager::Deinitialize()
{
    Super::Deinitialize();

    Clear();
}

void UStatefulObjectManager::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    Clear();
}

void UStatefulObjectManager::RegisterSpawner(AObjectSpawner* Spawner)
{
    ObjectSpawners.Add(Spawner);
}

void UStatefulObjectManager::RegisterObject(uint64 ObjectId, AActor* SpawnedActor)
{
    if (AMonster* Monster = Cast<AMonster>(SpawnedActor))
        Monsters.Add(ObjectId, Monster);
    else if (AP1Player* Player = Cast<AP1Player>(SpawnedActor))
        Players.Add(ObjectId, Player);
}

void UStatefulObjectManager::UnRegisterObject(uint64 ObjectId, EObjectType ObjectType)
{
    switch (ObjectType)
    {
    case EObjectType::Monster:
        Monsters.Remove(ObjectId);
        break;

    case EObjectType::Player:
        Players.Remove(ObjectId);
        break;
    default:
        UE_LOG(LogTemp, Log, TEXT("ObjectType 지정 안 됨"))
    }
}

AActor* UStatefulObjectManager::FindObject(uint64 ObjectId)
{
    if (AP1Player** FindPlayer = Players.Find(ObjectId))
    {
        return *FindPlayer;
    }
    else if (AMonster** FindMonster = Monsters.Find(ObjectId))
    {
        return *FindMonster;
    }

    return nullptr;
}

void UStatefulObjectManager::SpawnObject(const Protocol::ObjectInfo& InObjectInfo, int32 SpawnerId)
{
    if (ObjectSpawners.IsValidIndex(SpawnerId) == false)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not Found %d Spawner"), SpawnerId);
        return;
    }

    Protocol::ObjectType ObjectType = InObjectInfo.object_type();
    switch (ObjectType)
    {
    case Protocol::ObjectType::OBJECT_TYPE_MONSTER:
        SpawnMonster(InObjectInfo, SpawnerId);
        break;

    case Protocol::ObjectType::OBJECT_TYPE_PLAYER:
        SpawnPlayer(InObjectInfo, SpawnerId);
        break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("ObjectType 누락"))
        break;

    }

}

void UStatefulObjectManager::DespawnAllObjects(bool ExceptMine)
{
    UWorld* World = GetWorld();
    UMyPlayerData* MyPlayerData = World->GetGameInstance()->GetSubsystem<UMyPlayerData>();
    uint64 MyPlayerId = MyPlayerData->GetPlayerId();

    for (auto Pair : Players)
    {
        if (AP1Player* Player = Pair.Value)
        {
            if (MyPlayerId != Pair.Key)
                Player->Destroy();
        }
        
    }

    for (auto Pair : Monsters)
    {
        if (AMonster* Monster = Pair.Value)
        {
            Monster->Destroy();
        }

    }

    Clear();
}

void UStatefulObjectManager::DespawnObject(uint64 ObjectId)
{
    if (AP1Player** FindPlayer = Players.Find(ObjectId))
    {
        UnRegisterObject(ObjectId, EObjectType::Player);
        (*FindPlayer)->Destroy();
        return;
    }
    else if (AMonster** FindMonster = Monsters.Find(ObjectId))
    {
        UnRegisterObject(ObjectId, EObjectType::Monster);
        (*FindMonster)->Destroy();
        return;
    }
}

void UStatefulObjectManager::Clear()
{
    ObjectSpawners.Empty();
    Players.Empty();
    Monsters.Empty();
}

void UStatefulObjectManager::SpawnMonster(const Protocol::ObjectInfo& InObjectInfo, int32 SpawnerId)
{
    AObjectSpawner* Spawner = ObjectSpawners[SpawnerId];
    AMonster* OutMonster = nullptr;

    const uint64 ObjectId = InObjectInfo.object_id();
    if (Monsters.Find(ObjectId) != nullptr)
        return;

    if (Monsters.Find(ObjectId))
    {
        UE_LOG(LogTemp, Warning, TEXT("이미 존재하는 ObjectId를 가진 몬스터 스폰 시도"));
        return;
    }

    if (Spawner->SpawnMonster(OUT OutMonster, InObjectInfo) == false || OutMonster == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("몬스터 스폰 실패"));
        return;
    }

    // Add Monster
    RegisterObject(ObjectId, OutMonster);
}

void UStatefulObjectManager::SpawnPlayer(const Protocol::ObjectInfo& InObjectInfo, int32 SpawnerId)
{
    AObjectSpawner* Spawner = ObjectSpawners[SpawnerId];
    AP1Player* OutPlayer = nullptr;

    const uint64 ObjectId = InObjectInfo.object_id();
    if (Players.Find(ObjectId) != nullptr)
        return;

    if (Players.Find(ObjectId))
    {
        UE_LOG(LogTemp, Warning, TEXT("이미 존재하는 ObjectId를 가진 플레이어 스폰 시도"));
        return;
    }

    if (Spawner->SpawnPlayer(OUT OutPlayer, InObjectInfo) == false || OutPlayer == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("플레이어 스폰 실패"));
        return;
    }

    // Add Monster
    RegisterObject(ObjectId, OutPlayer);
}
