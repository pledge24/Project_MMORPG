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

/** 서버가 id로 관리하는 오브젝트를 월드 액터와 이어 붙이는 서브시스템이다. */
UCLASS()
class P1_API UP1StatefulObjectManager : public UWorldSubsystem
{
    GENERATED_BODY()

    //~ Begin USubsystem Interface
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    //~ Begin UWorldSubsystem Interface
public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    //~ End UWorldSubsystem Interface

    //~ Spawner Registry
public:
    void RegisterSpawner(AP1ObjectSpawner* Spawner);

protected:
    UPROPERTY()
    TArray<TObjectPtr<AP1ObjectSpawner>> ObjectSpawners;

    //~ Object Registry
public:
    void RegisterObject(uint64 ObjectId, AActor* SpawnedActor);
    void UnRegisterObject(uint64 ObjectId, EP1ObjectType ObjectType = EP1ObjectType::None);

    /** 찾지 못하면 nullptr을 돌려준다. */
    AActor* FindObject(uint64 ObjectId);

protected:
    void Clear();

    UPROPERTY()
    TMap<uint64, TObjectPtr<AP1Player>> Players;

    UPROPERTY()
    TMap<uint64, TObjectPtr<AP1Monster>> Monsters;

    //~ Spawn
public:
    void SpawnObject(const Protocol::ObjectInfo& InObjectInfo, int32 SpawnerId = 0);
    void DespawnAllObjects(bool ExceptMine);
    void DespawnObject(uint64 ObjectId);

protected:
    void SpawnMonster(const Protocol::ObjectInfo& InObjectInfo, int32 SpawnerId);
    void SpawnPlayer(const Protocol::ObjectInfo& InObjectInfo, int32 SpawnerId);
};
