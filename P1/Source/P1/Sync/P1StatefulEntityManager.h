#pragma once

#include "CoreMinimal.h"
#include "Sync/P1EntityType.h"
#include "Subsystems/WorldSubsystem.h"
#include "Protocol.pb.h"
#include "P1StatefulEntityManager.generated.h"

class AP1Monster;
class AP1Player;
class AP1EntitySpawner;
class UP1MyPlayerData;

/** 서버가 id로 관리하는 엔티티를 월드 액터와 이어 붙이는 서브시스템이다. */
UCLASS()
class P1_API UP1StatefulEntityManager : public UWorldSubsystem
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
    void RegisterSpawner(AP1EntitySpawner* Spawner);

protected:
    UPROPERTY()
    TArray<TObjectPtr<AP1EntitySpawner>> EntitySpawners;

    //~ Entity Registry
public:
    void RegisterEntity(uint64 EntityId, AActor* SpawnedActor);
    void UnRegisterEntity(uint64 EntityId, EP1EntityType EntityType = EP1EntityType::None);

    /** 찾지 못하면 nullptr을 돌려준다. */
    AActor* FindEntity(uint64 EntityId);

    /** 찾은 엔티티를 T로 캐스트한다. 없거나 타입이 다르면 nullptr. */
    template <typename T>
    T* FindEntityAs(uint64 EntityId)
    {
        return Cast<T>(FindEntity(EntityId));
    }

protected:
    void Clear();

    UPROPERTY()
    TMap<uint64, TObjectPtr<AP1Player>> Players;

    UPROPERTY()
    TMap<uint64, TObjectPtr<AP1Monster>> Monsters;

    //~ Spawn
public:
    void SpawnEntity(const Protocol::EntityInfo& InEntityInfo, int32 SpawnerId = 0);
    /** 내 플레이어만 남기고 모든 엔티티를 디스폰한다. */
    void DespawnAllEntities();
    void DespawnEntity(uint64 EntityId);

protected:
    void SpawnMonster(const Protocol::EntityInfo& InEntityInfo, int32 SpawnerId);
    void HandleMonsterDespawnReady(AP1Monster* Monster);
    void SpawnPlayer(const Protocol::EntityInfo& InEntityInfo, int32 SpawnerId);

    //~ Entity Packet Handlers
public:
    /** 패킷 핸들러가 수신 펌프에서 부른다. 게임 스레드 전용. 아래 핸들러도 같다. */
    void HandleSpawn(const Protocol::S_SPAWN& SpawnPkt);
    void HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt);
    void HandleMove(const Protocol::S_MOVE& MovePkt);

protected:
    void HandleMove(const Protocol::PosInfo& Info);

    //~ Gear Packet Handlers
public:
    /** 성공 응답이면 그 플레이어의 외형을 바꾼다. 내 플레이어의 슬롯과 스탯은 내 플레이어 데이터가 맡는다. */
    void HandleEquipGear(const Protocol::S_EQUIP_GEAR& EquipGearPkt);

    /** 성공 응답이면 그 플레이어의 외형을 바꾼다. 내 플레이어의 슬롯과 스탯은 내 플레이어 데이터가 맡는다. */
    void HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt);

    //~ Combat Packet Handlers
public:
    void HandleNormalAttack(const Protocol::S_NORMAL_ATTACK& NormalAttackPkt);

    /** 내 플레이어가 맞았으면 내 플레이어 데이터의 HP도 갱신한다. */
    void HandleHit(const Protocol::S_HIT& HitPkt);

    void HandleDie(const Protocol::S_DIE& DiePkt);

    /** 내 플레이어가 살아났으면 내 플레이어 데이터의 스탯도 갱신한다. */
    void HandleRespawn(const Protocol::S_RESPAWN& RespawnPkt);

protected:
    /** 게임 인스턴스가 없으면 nullptr. */
    UP1MyPlayerData* GetMyPlayerData() const;
};
