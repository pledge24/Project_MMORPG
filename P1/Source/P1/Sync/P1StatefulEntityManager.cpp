#include "Sync/P1StatefulEntityManager.h"

#include "Game/Progress/P1MyPlayerData.h"
#include "Sync/P1EntitySpawner.h"
#include "Sync/P1MoveSyncComponent.h"
#include "Game/Entities/P1Creature.h"
#include "Game/Entities/P1MyPlayer.h"
#include "Game/Entities/P1Player.h"
#include "Game/Entities/P1Monster.h"
#include "Utils/LogCategory.h"

void UP1StatefulEntityManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

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
        UE_LOG(LogP1Entity, Warning, TEXT("스포너 %d를 찾지 못함"), SpawnerId);
        return;
    }

    // 엔티티의 종류는 실린 세부 정보의 종류다.
    switch (InEntityInfo.details_case())
    {
    case Protocol::EntityInfo::kMonsterInfo:
        SpawnMonster(InEntityInfo, SpawnerId);
        break;

    case Protocol::EntityInfo::kPlayerInfo:
        SpawnPlayer(InEntityInfo, SpawnerId);
        break;
    default:
        UE_LOG(LogP1Entity, Warning, TEXT("세부 정보가 없는 엔티티 %llu는 스폰하지 않는다"), InEntityInfo.entity_id())
        break;

    }

}

void UP1StatefulEntityManager::DespawnAllEntities()
{
    UWorld* World = GetWorld();
    UP1MyPlayerData* MyPlayerData = World->GetGameInstance()->GetSubsystem<UP1MyPlayerData>();
    uint64 MyPlayerId = MyPlayerData->GetPlayerId();

    // 내 플레이어가 아직 스폰되지 않았거나 이미 빠졌을 수 있다. 그때는 전부 디스폰하고 다시 등록하지 않는다.
    TObjectPtr<AP1Player>* FoundMyPlayer = Players.Find(MyPlayerId);
    AP1Player* MyPlayer = FoundMyPlayer ? FoundMyPlayer->Get() : nullptr;

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

    if (MyPlayer)
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
        // 새 몬스터를 등록한다
        RegisterEntity(EntityId, NewMonster);
        NewMonster->OnDespawnReady.AddUObject(this, &UP1StatefulEntityManager::HandleMonsterDespawnReady);
    }
    else
    {
        UE_LOG(LogP1Entity, Warning, TEXT("몬스터 스폰 실패"));
    }
    
}

void UP1StatefulEntityManager::HandleMonsterDespawnReady(AP1Monster* Monster)
{
    if (UP1MoveSyncComponent* MoveSync = UP1MoveSyncComponent::FindOn(Monster))
        DespawnEntity(MoveSync->GetEntityId());
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
        // 새 플레이어를 등록한다
        RegisterEntity(EntityId, NewPlayer);
    }
    else
    {
        UE_LOG(LogP1Entity, Warning, TEXT("플레이어 스폰 실패"));
    }
}

    

void UP1StatefulEntityManager::HandleSpawn(const Protocol::S_SPAWN& SpawnPkt)
{
    for (auto& Entity : SpawnPkt.entities())
    {
        SpawnEntity(Entity);
    }
}

void UP1StatefulEntityManager::HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt)
{
    for (auto& EntityId : DespawnPkt.entity_ids())
    {
        DespawnEntity(EntityId);
    }
}

void UP1StatefulEntityManager::HandleMove(const Protocol::S_MOVE& MovePkt)
{
    for (auto& Move : MovePkt.moves())
        HandleMove(Move);
}

void UP1StatefulEntityManager::HandleMove(const Protocol::EntityMove& Move)
{
    if (AP1Creature* Creature = FindEntityAs<AP1Creature>(Move.entity_id()))
    {
        if (UP1MoveSyncComponent* MoveSync = UP1MoveSyncComponent::FindOn(Creature))
            MoveSync->PushToMoveQueue(Move.pos_info());
    }
}

void UP1StatefulEntityManager::HandleAttack(const Protocol::S_ATTACK& AttackPkt)
{
    AP1Creature* Creature = FindEntityAs<AP1Creature>(AttackPkt.entity_id());
    if (Creature == nullptr)
        return;

    Creature->S_NormalAttack(AttackPkt.combo(), AttackPkt.yaw());
}

void UP1StatefulEntityManager::HandleHit(const Protocol::S_HIT& HitPkt)
{
    AP1Creature* Creature = FindEntityAs<AP1Creature>(HitPkt.entity_id());
    if (Creature == nullptr)
        return;

    // 피격 연출과 HP 갱신
    Creature->S_Hit(HitPkt.damage(), HitPkt.hp());

    if (Creature->IsMyPlayer())
    {
        if (UP1MyPlayerData* MyPlayerData = GetMyPlayerData())
            MyPlayerData->ApplyStat(Protocol::STAT_TYPE_HP, HitPkt.hp());
    }
}

void UP1StatefulEntityManager::HandleDie(const Protocol::S_DIE& DiePkt)
{
    AP1Creature* Creature = FindEntityAs<AP1Creature>(DiePkt.entity_id());
    if (Creature == nullptr)
        return;

    Creature->S_Die();
}

UP1MyPlayerData* UP1StatefulEntityManager::GetMyPlayerData() const
{
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
        return GameInstance->GetSubsystem<UP1MyPlayerData>();

    return nullptr;
}

void UP1StatefulEntityManager::HandleEquipGear(const Protocol::S_EQUIP_GEAR& EquipGearPkt)
{
    if (EquipGearPkt.success() == false)
        return;

    // 서버가 처리 결과로 보낸 장비 부위와 그 부위의 아이템
    if (AP1Player* Player = FindEntityAs<AP1Player>(EquipGearPkt.entity_id()))
        Player->ApplyGear(EquipGearPkt.gear_type(), EquipGearPkt.template_id());
}

void UP1StatefulEntityManager::HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt)
{
    if (UnequipGearPkt.success() == false)
        return;

    // 서버가 처리 결과로 보낸 장비 부위와 그 부위의 아이템
    if (AP1Player* Player = FindEntityAs<AP1Player>(UnequipGearPkt.entity_id()))
        Player->ApplyGear(UnequipGearPkt.gear_type(), UnequipGearPkt.template_id());
}

void UP1StatefulEntityManager::HandleEnterRoom(const Protocol::S_ENTER_ROOM& EnterRoomPkt)
{
    if (EnterRoomPkt.result() != Protocol::ENTRY_RESULT_SUCCESS)
        return;

    // 앞 룸의 엔티티를 모두 지운다. 같은 맵 안의 포털 이동과 리스폰을 입장 유형으로 가르지 않는다.
    DespawnAllEntities();

    // 맵을 새로 열었으면 내 플레이어가 아직 없다. 내 엔티티는 스폰이 아니라 이 응답에만 온다.
    const Protocol::EntityInfo& MyEntity = EnterRoomPkt.my_entity();
    const TObjectPtr<AP1Player>* Found = Players.Find(MyEntity.entity_id());
    AP1Creature* MyPlayer = Found ? Found->Get() : nullptr;
    if (MyPlayer == nullptr)
    {
        SpawnEntity(MyEntity);
        return;
    }

    // 같은 맵 안에서 옮기면 내 플레이어를 남겨 두고 옮긴다. 위치를 먼저 옮긴 뒤 사망을 푼다. OnRespawn을 받는 쪽이 옮겨진 위치를 본다.
    if (UP1MoveSyncComponent* MoveSync = UP1MoveSyncComponent::FindOn(MyPlayer))
    {
        MoveSync->SetClientPos(MyEntity.pos_info());
        MoveSync->SetServerPos(MyEntity.pos_info());
    }

    if (MyPlayer->IsDead())
        MyPlayer->S_Respawn();
}
