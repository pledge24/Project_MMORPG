#include "Sync/P1EntitySpawner.h"
#include "Game/Entities/P1Monster.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Sync/P1StatefulEntityManager.h"
#include "Sync/P1MoveSyncComponent.h"
#include "Utils/LogCategory.h"
#include "Game/Data/P1MonsterAssetData.h"

namespace
{
    /** 이동 동기화 컴포넌트를 붙인다. FinishSpawning 전에 불러야 액터의 BeginPlay와 함께 시작한다. */
    UP1MoveSyncComponent* AttachMoveSync(AActor* Actor, EP1MoveSyncMode Mode)
    {
        UP1MoveSyncComponent* MoveSync = NewObject<UP1MoveSyncComponent>(Actor, TEXT("MoveSync"));
        MoveSync->SetMode(Mode);
        Actor->AddInstanceComponent(MoveSync);
        MoveSync->RegisterComponent();
        return MoveSync;
    }
}

void AP1EntitySpawner::BeginPlay()
{
    Super::BeginPlay();

    if (MonsterDataTable == nullptr)
    {
        UE_LOG(LogP1Entity, Warning, TEXT("AP1EntitySpawner에서 MonsterDataTable 누락"));
        return;
    }

    if (UP1StatefulEntityManager* EntityManager = GetWorld()->GetSubsystem<UP1StatefulEntityManager>())
    {
        EntityManager->RegisterSpawner(this);
    }

}

AActor* AP1EntitySpawner::SpawnMonster(int32 TemplateId, const FTransform& Transform)
{
    FVector Location = Transform.GetLocation();
    FRotator Rotation = Transform.GetRotation().Rotator();

    return SpawnMonster(TemplateId, Location, Rotation);
}

AActor* AP1EntitySpawner::SpawnMonster(const Protocol::EntityInfo& InEntityInfo)
{
    int32 TemplateId = InEntityInfo.monster_info().template_id();
    const Protocol::PosInfo& PosInfo_ = InEntityInfo.pos_info();
    const Protocol::Vector& Pos = PosInfo_.pos();

    FVector Location = FVector(Pos.x(), Pos.y(), Pos.z());
    FRotator Rotation = FRotator(0, PosInfo_.yaw(), 0);

    return SpawnMonster(TemplateId, Location, Rotation, InEntityInfo);
}

AActor* AP1EntitySpawner::SpawnMonster(int32 TemplateId, const FVector& SpawnLocation, const FRotator& SpawnRotation, TOptional<Protocol::EntityInfo> ServerInfo)
{
    if (MonsterDataTable == nullptr)
        return nullptr;

    FP1MonsterData MonsterData;
    if (GetMonsterData(TemplateId, MonsterData) == false)
        return nullptr;

    TSubclassOf<AP1Monster> MonsterBPClass = GetMonsterClass(TemplateId);
    if (MonsterBPClass == nullptr)
        return nullptr;

    AP1Monster* OutMonster = GetWorld()->SpawnActorDeferred<AP1Monster>(
        MonsterBPClass,
        FTransform(SpawnRotation, SpawnLocation),
        nullptr, 
        nullptr, 
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    // 스폰 전에 몬스터 데이터 설정
    if (OutMonster != nullptr)
    {
        UP1MoveSyncComponent* MoveSync = AttachMoveSync(OutMonster, EP1MoveSyncMode::Remote);

        if (ServerInfo.IsSet())
        {
            MoveSync->InitPos(ServerInfo.GetValue().pos_info());
            OutMonster->Initialize(ServerInfo.GetValue());
        }

        OutMonster->SetDefaultMonsterData(MonsterData);
        OutMonster->FinishSpawning(FTransform(SpawnRotation, SpawnLocation));
    }

    return OutMonster;
}

bool AP1EntitySpawner::GetMonsterData(int32 TemplateId, FP1MonsterData& OutMonsterData)
{
    FName RowName = *FString::FromInt(TemplateId);

    if (MonsterDataTable == nullptr)
    {
        UE_LOG(LogP1Entity, Warning, TEXT("MonsterDataTable이 비어 있음"));
        return false;
    }

    FP1MonsterData* FoundRow = MonsterDataTable->FindRow<FP1MonsterData>(RowName, TEXT("GetRowData"), true);

    if (!FoundRow)
    {
        UE_LOG(LogP1Entity, Warning, TEXT("RowName{%s}에 해당하는 행이 없음"), *RowName.ToString());
        return false;
    }

    OutMonsterData = *FoundRow;

    return true;
}

TSubclassOf<AP1Monster> AP1EntitySpawner::GetMonsterClass(int32 TemplateId) const
{
    if (MonsterAssetTable == nullptr)
    {
        UE_LOG(LogP1Entity, Warning, TEXT("AP1EntitySpawner에 MonsterAssetTable이 지정되지 않음"));
        return nullptr;
    }

    const FP1MonsterAssetData* AssetData = MonsterAssetTable->FindRow<FP1MonsterAssetData>(
        FName(*FString::FromInt(TemplateId)), TEXT("AP1EntitySpawner::GetMonsterClass"));
    if (AssetData == nullptr)
        return nullptr;

    return AssetData->MonsterClass.LoadSynchronous();
}

AActor* AP1EntitySpawner::SpawnPlayer(const Protocol::EntityInfo& InEntityInfo)
{
    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance == nullptr)
        return nullptr;

    UWorld* World = GetWorld();
    UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>();
    uint64 MyPlayerId = MyPlayerData->GetPlayerId();
    bool IsMine = MyPlayerId == InEntityInfo.entity_id();

    FVector SpawnLocation(InEntityInfo.pos_info().pos().x(), InEntityInfo.pos_info().pos().y(), InEntityInfo.pos_info().pos().z());
    FRotator SpawnRotation(0.f, InEntityInfo.pos_info().yaw(), 0.f);

    AP1Player* OutPlayer = nullptr;
    if (IsMine)
    {
        if (!MyPlayerClass)
        {
            UE_LOG(LogP1Entity, Warning, TEXT("MyPlayerClass가 설정되지 않았습니다"));
            return nullptr;
        }

        OutPlayer = World->SpawnActorDeferred<AP1Player>(
            MyPlayerClass,
            FTransform(SpawnRotation, SpawnLocation),
            nullptr,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn
        );
    }
    else
    {
        if (!OtherPlayerClass)
        {
            UE_LOG(LogP1Entity, Warning, TEXT("OtherPlayerClass가 설정되지 않았습니다"));
            return nullptr;
        }

        OutPlayer = World->SpawnActorDeferred<AP1Player>(
            OtherPlayerClass,
            FTransform(SpawnRotation, SpawnLocation),
            nullptr,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn
        );

    }

    if (OutPlayer != nullptr)
    {
        // 플레이어 데이터 설정(스폰 전 후로)
        // 이름은 Initialize도 다시 넣지만 여기서 먼저 넣어야 한다. FinishSpawning 안의 BeginPlay가
        // 네임플레이트를 바인딩하면서 이름을 읽는데, Initialize는 그 뒤에 불린다.
        FString PlayerName = InEntityInfo.player_info().name().c_str();
        OutPlayer->SetPlayerName(FText::FromString(PlayerName));
        // 서버 위치를 보간 목표로도 넣어 이동 방향까지 채운다. 몬스터는 위치만 넣는다.
        UP1MoveSyncComponent* MoveSync = AttachMoveSync(OutPlayer, IsMine ? EP1MoveSyncMode::MyPlayer : EP1MoveSyncMode::Remote);
        MoveSync->InitPos(InEntityInfo.pos_info());
        MoveSync->SetServerPos(InEntityInfo.pos_info());
        OutPlayer->FinishSpawning(FTransform(SpawnRotation, SpawnLocation));
        OutPlayer->Initialize(InEntityInfo);

        // 구독자가 초기화를 마친 내 플레이어를 받도록 Initialize 뒤에 알린다.
        if (IsMine)
        {
            if (AP1MyPlayer* MyPlayer = Cast<AP1MyPlayer>(OutPlayer))
                MyPlayerData->OnMyPlayerSpawned.Broadcast(MyPlayer);
        }
    }
    else
    {
        UE_LOG(LogP1Entity, Warning, TEXT("플레이어 SpawnActorDeferred<> NullPtr 반환"));
    }

    return OutPlayer;
}


