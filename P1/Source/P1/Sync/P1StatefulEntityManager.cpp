#include "Sync/P1StatefulEntityManager.h"

#include "Core/P1MyPlayerData.h"
#include "Sync/P1EntitySpawner.h"
#include "Game/Entities/P1Player.h"
#include "Utils/LogCategory.h"

void UP1StatefulEntityManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // BP로 설정한 property가 없음
    //UWorld* World = GetWorld();
    //if (AP1EntitySpawner* EntitySpawner = World->SpawnActor<AP1EntitySpawner>())
    //{
    //    RegisterSpawner(EntitySpawner);
    //}
    Clear();
}

void UP1StatefulEntityManager::Deinitialize()
{
    Super::Deinitialize();

    Clear();
}

void UP1StatefulEntityManager::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
}

void UP1StatefulEntityManager::RegisterSpawner(AP1EntitySpawner* Spawner)
{
    EntitySpawners.Add(Spawner);
}

void UP1StatefulEntityManager::RegisterEntity(uint64 EntityId, AActor* SpawnedActor)
{
    if (AP1Monster* Monster = Cast<AP1Monster>(SpawnedActor))
        Monsters.Add(EntityId, Monster);
    else if (AP1Player* Player = Cast<AP1Player>(SpawnedActor))
        Players.Add(EntityId, Player);

    //UE_LOG(LogP1Entity, Log, TEXT("엔티티 {%d} 등록됨"), EntityId);
}

void UP1StatefulEntityManager::UnRegisterEntity(uint64 EntityId, EP1EntityType EntityType)
{
    switch (EntityType)
    {
    case EP1EntityType::Monster:
        Monsters.Remove(EntityId);
        break;

    case EP1EntityType::Player:
        Players.Remove(EntityId);
        break;
    default:
        UE_LOG(LogP1Entity, Log, TEXT("EntityType 지정 안 됨"))
    }
}

AActor* UP1StatefulEntityManager::FindEntity(uint64 EntityId)
{
    if (TObjectPtr<AP1Player>* FindPlayer = Players.Find(EntityId))
    {
        return *FindPlayer;
    }
    else if (TObjectPtr<AP1Monster>* FindMonster = Monsters.Find(EntityId))
    {
        return *FindMonster;
    }

    UE_LOG(LogP1Entity, Warning, TEXT("해당 엔티티(Id:%d)를 EntityManager에서 찾지 못했습니다"), (int32)EntityId);

    return nullptr;
}

void UP1StatefulEntityManager::SpawnEntity(const Protocol::EntityInfo& InEntityInfo, int32 SpawnerId)
{
    if (EntitySpawners.IsValidIndex(SpawnerId) == false)
    {
        UE_LOG(LogP1Entity, Warning, TEXT("Not Found %d Spawner"), SpawnerId);
        return;
    }

    Protocol::EntityType EntityType = InEntityInfo.entity_type();
    switch (EntityType)
    {
    case Protocol::EntityType::ENTITY_TYPE_MONSTER:
        SpawnMonster(InEntityInfo, SpawnerId);
        break;

    case Protocol::EntityType::ENTITY_TYPE_PLAYER:
        SpawnPlayer(InEntityInfo, SpawnerId);
        break;
    default:
        UE_LOG(LogP1Entity, Warning, TEXT("EntityType 누락"))
        break;

    }

}

void UP1StatefulEntityManager::DespawnAllEntities(bool ExceptMine)
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
    RegisterEntity(MyPlayerId, MyPlayer);
}

void UP1StatefulEntityManager::DespawnEntity(uint64 EntityId)
{
    if (TObjectPtr<AP1Player>* FindPlayer = Players.Find(EntityId))
    {
        UnRegisterEntity(EntityId, EP1EntityType::Player);
        (*FindPlayer)->Destroy();
        return;
    }
    else if (TObjectPtr<AP1Monster>* FindMonster = Monsters.Find(EntityId))
    {
        UnRegisterEntity(EntityId, EP1EntityType::Monster);
        (*FindMonster)->Destroy();
        return;
    }
}

void UP1StatefulEntityManager::Clear()
{
    Players.Empty();
    Monsters.Empty();
}

void UP1StatefulEntityManager::SpawnMonster(const Protocol::EntityInfo& InEntityInfo, int32 SpawnerId)
{
    AP1EntitySpawner* Spawner = EntitySpawners[SpawnerId];

    const uint64 EntityId = InEntityInfo.entity_id();
    if (Monsters.Find(EntityId) != nullptr)
        return;

    if (Monsters.Find(EntityId))
    {
        UE_LOG(LogP1Entity, Warning, TEXT("이미 존재하는 EntityId를 가진 몬스터 스폰 시도"));
        return;
    }

    if (AP1Monster* NewMonster = Cast<AP1Monster>(Spawner->SpawnMonster(InEntityInfo)))
    {
        // Register New Monster
        RegisterEntity(EntityId, NewMonster);
    }
    else
    {
        UE_LOG(LogP1Entity, Warning, TEXT("몬스터 스폰 실패"));
    }
    
}

void UP1StatefulEntityManager::SpawnPlayer(const Protocol::EntityInfo& InEntityInfo, int32 SpawnerId)
{
    AP1EntitySpawner* Spawner = EntitySpawners[SpawnerId];

    const uint64 EntityId = InEntityInfo.entity_id();
    if (Players.Find(EntityId) != nullptr)
        return;

    if (Players.Find(EntityId))
    {
        UE_LOG(LogP1Entity, Warning, TEXT("이미 존재하는 EntityId를 가진 플레이어 스폰 시도"));
        return;
    }

    if (AP1Player* NewPlayer = Cast<AP1Player>(Spawner->SpawnPlayer(InEntityInfo)))
    {
        // Register New Player
        RegisterEntity(EntityId, NewPlayer);
    }
    else
    {
        UE_LOG(LogP1Entity, Warning, TEXT("플레이어 스폰 실패"));
    }
}

    
