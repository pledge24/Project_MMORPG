#pragma once

#include "CoreMinimal.h"
#include "Sync/P1EntityType.h"
#include "Subsystems/WorldSubsystem.h"
#include "Network/Item.pb.h"
#include "Entity.pb.h"
#include "Combat.pb.h"
#include "Entry.pb.h"
#include "P1StatefulEntityManager.generated.h"

class AP1Monster;
class AP1Player;
class AP1EntitySpawner;
class UP1MyPlayerData;

/**
 * 서버 엔티티와 월드 액터를 잇는 관리자.
 *
 * 서버가 id로 관리하는 엔티티를 월드의 액터와 이어 붙이고, 엔티티에 관한 패킷을 그 액터에 전달한다.
 * 게임 스레드 전용.
 */
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
    /** 등록한 순서가 스포너 id가 된다. */
    void RegisterSpawner(AP1EntitySpawner* Spawner);

protected:
    UPROPERTY()
    TArray<TObjectPtr<AP1EntitySpawner>> EntitySpawners;

    //~ Entity Registry
public:
    void RegisterEntity(uint64 EntityId, AActor* SpawnedActor);
    /** EntityType의 목록에서 뺀다. None이면 아무것도 빼지 않는다. */
    void UnRegisterEntity(uint64 EntityId, EP1EntityType EntityType = EP1EntityType::None);

    /** 없으면 경고를 남기고 nullptr. */
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
    /** SpawnerId 번 스포너로 스폰한다. 이미 등록된 id면 무시한다. */
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
    void HandleSpawn(const Protocol::S_SPAWN& SpawnPkt);
    void HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt);
    void HandleMove(const Protocol::S_MOVE& MovePkt);

protected:
    void HandleMove(const Protocol::EntityMove& Move);

    //~ Room Packet Handlers
public:
    /**
     * 앞 룸의 엔티티를 지우고 내 플레이어를 입장 위치에 둔다. 내 플레이어가 없으면 스폰하고, 사망해 있으면 되살린다.
     * 내 플레이어 데이터가 내 엔티티 번호를 먼저 저장해야 한다.
     */
    void HandleEnterRoom(const Protocol::S_ENTER_ROOM& EnterRoomPkt);

    //~ Gear Packet Handlers
public:
    /** 내 플레이어를 포함한 그 플레이어의 외형을 바꾼다. 내 플레이어의 슬롯과 스탯은 내 플레이어 데이터가 맡는다. */
    void HandleGearAppearance(const Protocol::S_GEAR_APPEARANCE& GearAppearancePkt);

    //~ Combat Packet Handlers
public:
    /** 다른 크리처의 공격을 재생한다. 내 공격은 서버가 알리지 않는다. */
    void HandleAttack(const Protocol::S_ATTACK& AttackPkt);

    /** 내 플레이어가 맞았으면 내 플레이어 데이터의 HP도 갱신한다. */
    void HandleHit(const Protocol::S_HIT& HitPkt);

    void HandleDie(const Protocol::S_DIE& DiePkt);

protected:
    /** 게임 인스턴스가 없으면 nullptr. */
    UP1MyPlayerData* GetMyPlayerData() const;
};
