#include "Entities/P1StatefulObjectManager.h"

#include "Core/P1MyPlayerData.h"
#include "Entities/P1ObjectSpawner.h"
#include "Characters/P1Player.h"
#include "Utils/LogCategory.h"

void UP1StatefulObjectManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // BP로 설정한 property가 없음
    //UWorld* World = GetWorld();
    //if (AP1ObjectSpawner* ObjectSpawner = World->SpawnActor<AP1ObjectSpawner>())
    //{
    //    RegisterSpawner(ObjectSpawner);
    //}
    Clear();
}

void UP1StatefulObjectManager::Deinitialize()
{
    Super::Deinitialize();

    Clear();
}

void UP1StatefulObjectManager::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
}

void UP1StatefulObjectManager::RegisterSpawner(AP1ObjectSpawner* Spawner)
{
    ObjectSpawners.Add(Spawner);
}

void UP1StatefulObjectManager::RegisterObject(uint64 ObjectId, AActor* SpawnedActor)
{
    if (AP1Monster* Monster = Cast<AP1Monster>(SpawnedActor))
        Monsters.Add(ObjectId, Monster);
    else if (AP1Player* Player = Cast<AP1Player>(SpawnedActor))
        Players.Add(ObjectId, Player);

    //UE_LOG(LogP1Entity, Log, TEXT("Object {%d} 등록됨"), ObjectId);
}

void UP1StatefulObjectManager::UnRegisterObject(uint64 ObjectId, EP1ObjectType ObjectType)
{
    switch (ObjectType)
    {
    case EP1ObjectType::Monster:
        Monsters.Remove(ObjectId);
        break;

    case EP1ObjectType::Player:
        Players.Remove(ObjectId);
        break;
    default:
        UE_LOG(LogP1Entity, Log, TEXT("ObjectType 지정 안 됨"))
    }
}

AActor* UP1StatefulObjectManager::FindObject(uint64 ObjectId)
{
    if (TObjectPtr<AP1Player>* FindPlayer = Players.Find(ObjectId))
    {
        return *FindPlayer;
    }
    else if (TObjectPtr<AP1Monster>* FindMonster = Monsters.Find(ObjectId))
    {
        return *FindMonster;
    }

    UE_LOG(LogP1Entity, Warning, TEXT("해당 Object(Id:%d)를 ObjectManager에서 찾지 못했습니다"), (int32)ObjectId);

    return nullptr;
}

void UP1StatefulObjectManager::SpawnObject(const Protocol::ObjectInfo& InObjectInfo, int32 SpawnerId)
{
    if (ObjectSpawners.IsValidIndex(SpawnerId) == false)
    {
        UE_LOG(LogP1Entity, Warning, TEXT("Not Found %d Spawner"), SpawnerId);
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
        UE_LOG(LogP1Entity, Warning, TEXT("ObjectType 누락"))
        break;

    }

}

void UP1StatefulObjectManager::DespawnAllObjects(bool ExceptMine)
{
    UWorld* World = GetWorld();
    UP1MyPlayerData* MyPlayerData = World->GetGameInstance()->GetSubsystem<UP1MyPlayerData>();
    uint64 MyPlayerId = MyPlayerData->GetPlayerId();
    AP1Player* MyPlayer = Players[MyPlayerId];

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
        if (AP1Monster* Monster = Pair.Value)
        {
            Monster->Destroy();
        }

    }

    Clear();
    RegisterObject(MyPlayerId, MyPlayer);
}

void UP1StatefulObjectManager::DespawnObject(uint64 ObjectId)
{
    if (TObjectPtr<AP1Player>* FindPlayer = Players.Find(ObjectId))
    {
        UnRegisterObject(ObjectId, EP1ObjectType::Player);
        (*FindPlayer)->Destroy();
        return;
    }
    else if (TObjectPtr<AP1Monster>* FindMonster = Monsters.Find(ObjectId))
    {
        UnRegisterObject(ObjectId, EP1ObjectType::Monster);
        (*FindMonster)->Destroy();
        return;
    }
}

void UP1StatefulObjectManager::Clear()
{
    Players.Empty();
    Monsters.Empty();
}

void UP1StatefulObjectManager::SpawnMonster(const Protocol::ObjectInfo& InObjectInfo, int32 SpawnerId)
{
    AP1ObjectSpawner* Spawner = ObjectSpawners[SpawnerId];

    const uint64 ObjectId = InObjectInfo.object_id();
    if (Monsters.Find(ObjectId) != nullptr)
        return;

    if (Monsters.Find(ObjectId))
    {
        UE_LOG(LogP1Entity, Warning, TEXT("이미 존재하는 ObjectId를 가진 몬스터 스폰 시도"));
        return;
    }

    if (AP1Monster* NewMonster = Cast<AP1Monster>(Spawner->SpawnMonster(InObjectInfo)))
    {
        // Register New Monster
        RegisterObject(ObjectId, NewMonster);
    }
    else
    {
        UE_LOG(LogP1Entity, Warning, TEXT("몬스터 스폰 실패"));
    }
    
}

void UP1StatefulObjectManager::SpawnPlayer(const Protocol::ObjectInfo& InObjectInfo, int32 SpawnerId)
{
    AP1ObjectSpawner* Spawner = ObjectSpawners[SpawnerId];

    const uint64 ObjectId = InObjectInfo.object_id();
    if (Players.Find(ObjectId) != nullptr)
        return;

    if (Players.Find(ObjectId))
    {
        UE_LOG(LogP1Entity, Warning, TEXT("이미 존재하는 ObjectId를 가진 플레이어 스폰 시도"));
        return;
    }

    if (AP1Player* NewPlayer = Cast<AP1Player>(Spawner->SpawnPlayer(InObjectInfo)))
    {
        // Register New Player
        RegisterObject(ObjectId, NewPlayer);
    }
    else
    {
        UE_LOG(LogP1Entity, Warning, TEXT("플레이어 스폰 실패"));
    }
}

    
