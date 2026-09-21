#pragma once

#include "CoreMinimal.h"
#include "Entities/P1ObjectType.h"
#include "Subsystems/WorldSubsystem.h"
#include "Protocol.pb.h"
#include "P1StatefulObjectManager.generated.h"

class APlayerSpawner;
class AP1Monster;
class AP1Player;
class AP1ObjectSpawner;

/**
 * 
 */
UCLASS()
class P1_API UP1StatefulObjectManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

public:
    void RegisterSpawner(AP1ObjectSpawner* Spawner);
    void RegisterObject(uint64 ObjectId, AActor* SpawnedActor);
    void UnRegisterObject(uint64 ObjectId, EP1ObjectType ObjectType = EP1ObjectType::None);

    AActor* FindObject(uint64 ObjectId);

    void SpawnObject(const Protocol::ObjectInfo& InObjectInfo, int32 SpawnerId = 0);
    void DespawnAllObjects(bool ExceptMine);
    void DespawnObject(uint64 ObjectId);

protected:
    void Clear();
    void SpawnMonster(const Protocol::ObjectInfo& InObjectInfo, int32 SpawnerId);
    void SpawnPlayer(const Protocol::ObjectInfo& InObjectInfo, int32 SpawnerId);

protected:
    UPROPERTY()
    TArray<TObjectPtr<AP1ObjectSpawner>> ObjectSpawners;

    UPROPERTY()
    TMap<uint64, TObjectPtr<AP1Player>> Players;

    UPROPERTY()
    TMap<uint64, TObjectPtr<AP1Monster>> Monsters;
};
