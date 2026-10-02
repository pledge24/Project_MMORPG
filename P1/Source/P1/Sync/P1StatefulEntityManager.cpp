#include "Sync/P1StatefulEntityManager.h"

#include "Game/Progress/P1MyPlayerData.h"
#include "Sync/P1EntitySpawner.h"
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
    DespawnEntity(Monster->GetPosInfo()->entity_id());
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
    for (auto& Info : MovePkt.info())
        HandleMove(Info);
}

void UP1StatefulEntityManager::HandleMove(const Protocol::PosInfo& Info)
{
    if (AP1Creature* Creature = FindEntityAs<AP1Creature>(Info.entity_id()))
    {
        Creature->PushToMoveQueue(Info);
    }
}

void UP1StatefulEntityManager::HandleNormalAttack(const Protocol::S_NORMAL_ATTACK& NormalAttackPkt)
{
    AP1Creature* Creature = FindEntityAs<AP1Creature>(NormalAttackPkt.entity_id());
    if (Creature == nullptr)
        return;

    Creature->S_NormalAttack(NormalAttackPkt.combo(), NormalAttackPkt.yaw());
}

void UP1StatefulEntityManager::HandleHit(const Protocol::S_HIT& HitPkt)
{
    AP1Creature* Creature = FindEntityAs<AP1Creature>(HitPkt.entity_id());
    if (Creature == nullptr)
        return;

    // 피격 연출과 HP 갱신
    Creature->S_Hit(HitPkt.damage(), HitPkt.updated_hp());

    if (Creature->IsMyPlayer())
    {
        if (UP1MyPlayerData* MyPlayerData = GetMyPlayerData())
            MyPlayerData->ApplyStat(Protocol::STAT_TYPE_HP, HitPkt.updated_hp());
    }
}

void UP1StatefulEntityManager::HandleDie(const Protocol::S_DIE& DiePkt)
{
    AP1Creature* Creature = FindEntityAs<AP1Creature>(DiePkt.entity_id());
    if (Creature == nullptr)
        return;

    Creature->S_Die();
}

void UP1StatefulEntityManager::HandleRespawn(const Protocol::S_RESPAWN& RespawnPkt)
{
    if (RespawnPkt.success() == false)
    {
        UE_LOG(LogP1Network, Warning, TEXT("서버에서 리스폰 실패: %hs"), RespawnPkt.error_message().c_str());
        return;
    }

    // 서버는 같은 액터가 살아나는 것으로 다룬다. 새로 스폰하지 않는다.
    AP1Creature* Creature = FindEntityAs<AP1Creature>(RespawnPkt.entity_id());
    if (Creature == nullptr)
        return;

    Creature->S_Respawn(RespawnPkt.pos_info());

    if (Creature->IsMyPlayer())
    {
        if (UP1MyPlayerData* MyPlayerData = GetMyPlayerData())
            MyPlayerData->ApplyStats(RespawnPkt.updated_stat());
    }
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
        Player->ApplyGear(EquipGearPkt.slot_id(), EquipGearPkt.template_id());
}

void UP1StatefulEntityManager::HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt)
{
    if (UnequipGearPkt.success() == false)
        return;

    // 서버가 처리 결과로 보낸 장비 부위와 그 부위의 아이템
    if (AP1Player* Player = FindEntityAs<AP1Player>(UnequipGearPkt.entity_id()))
        Player->ApplyGear(UnequipGearPkt.slot_id(), UnequipGearPkt.template_id());
}

void UP1StatefulEntityManager::HandleEnterRoom(const Protocol::S_ENTER_ROOM& EnterRoomPkt)
{
    if (EnterRoomPkt.success() == false)
        return;

    // 다른 룸으로 리스폰하는 경우도 룸이 한 맵 안의 논리 분할이라 같은 맵 이동과 같은 처리다.
    if (EnterRoomPkt.enter_type() != Protocol::ENTER_TYPE_SAME_MAP_TRANSFER
        && EnterRoomPkt.enter_type() != Protocol::ENTER_TYPE_RESPAWN)
        return;

    DespawnAllEntities();

    if (EnterRoomPkt.has_enter_pos() == false)
        return;

    UP1MyPlayerData* MyPlayerData = GetMyPlayerData();
    if (MyPlayerData == nullptr)
        return;

    if (AP1MyPlayer* MyPlayer = FindEntityAs<AP1MyPlayer>(MyPlayerData->GetPlayerId()))
    {
        MyPlayer->SetClientPos(EnterRoomPkt.enter_pos());
        MyPlayer->SetServerPos(EnterRoomPkt.enter_pos());
    }
}
