// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EObject.h"
#include "Subsystems/WorldSubsystem.h"
#include "Protocol.pb.h"
#include "StatefulObjectManager.generated.h"

class APlayerSpawner;
class AMonster;
class AP1Player;
class AObjectSpawner;

/**
 * 
 */
UCLASS()
class P1_API UStatefulObjectManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

public:
    void RegisterSpawner(AObjectSpawner* Spawner);
    void RegisterObject(uint64 ObjectId, AActor* SpawnedActor);
    void UnRegisterObject(uint64 ObjectId, EObjectType ObjectType = EObjectType::None);

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
    TArray<AObjectSpawner*> ObjectSpawners;

    UPROPERTY()
    TMap<uint64, AP1Player*> Players;

    UPROPERTY()
    TMap<uint64, AMonster*> Monsters;
};
