#include "Entities/P1ObjectSpawner.h"
#include "Characters/P1Monster.h"
#include "Core/P1MyPlayerData.h"
#include "Core/P1GameInstance.h"
#include "Entities/P1StatefulObjectManager.h"
#include "Utils/LogCategory.h"

AP1ObjectSpawner::AP1ObjectSpawner()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AP1ObjectSpawner::BeginPlay()
{
    Super::BeginPlay();

    if (MonsterDataTable == nullptr)
    {
        UE_LOG(LogP1Entity, Warning, TEXT("AMonsterSpawner에서 MonsterDataTable 누락"));
        return;
    }

    if (UP1StatefulObjectManager* ObjectManager = GetWorld()->GetSubsystem<UP1StatefulObjectManager>())
    {
        ObjectManager->RegisterSpawner(this);
    }

}

AActor* AP1ObjectSpawner::SpawnMonster(int32 TemplateId, const FTransform& Transform)
{
    FVector Location = Transform.GetLocation();
    FRotator Rotation = Transform.GetRotation().Rotator();

    return SpawnMonster(TemplateId, Location, Rotation);
}

AActor* AP1ObjectSpawner::SpawnMonster(const Protocol::ObjectInfo& InObjectInfo)
{
    int32 TemplateId = InObjectInfo.monster_info().template_id();
    const Protocol::PosInfo& PosInfo_ = InObjectInfo.pos_info();
    const Protocol::Vector& Pos = PosInfo_.pos();

    FVector Location = FVector(Pos.x(), Pos.y(), Pos.z());
    FRotator Rotation = FRotator(0, PosInfo_.yaw(), 0);

    return SpawnMonster(TemplateId, Location, Rotation, InObjectInfo);
}

AActor* AP1ObjectSpawner::SpawnMonster(int32 TemplateId, const FVector& SpawnLocation, const FRotator& SpawnRotation, TOptional<Protocol::ObjectInfo> ServerInfo)
{
    if (MonsterDataTable == nullptr)
        return nullptr;

    FP1MonsterData MonsterData;
    if (GetMonsterData(TemplateId, MonsterData) == false)
        return nullptr;

    TSubclassOf<AP1Monster> MonsterBPClass = MonsterData.MonsterClass.Get();

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
        if (ServerInfo.IsSet())
        {
            OutMonster->Initialize(ServerInfo.GetValue());
        }

        OutMonster->SetDefaultMonsterData(MonsterData);
        OutMonster->FinishSpawning(FTransform(SpawnRotation, SpawnLocation));
    }

    return OutMonster;
}

bool AP1ObjectSpawner::GetMonsterData(int32 TemplateId, FP1MonsterData& OutMonsterData)
{
    FName RowName = *FString::FromInt(TemplateId);

    if (MonsterDataTable == nullptr)
    {
        UE_LOG(LogP1Entity, Warning, TEXT("MonsterDataTable Is Null"));
        return false;
    }

    FP1MonsterData* FoundRow = MonsterDataTable->FindRow<FP1MonsterData>(RowName, TEXT("GetRowData"), true);

    if (!FoundRow)
    {
        UE_LOG(LogP1Entity, Warning, TEXT("RowName{%s} Is Not Exist"), *RowName.ToString());
        return false;
    }

    if (!FoundRow->MonsterClass)
        FoundRow->MonsterClass.LoadSynchronous();

    OutMonsterData = *FoundRow;

    return true;
}

AActor* AP1ObjectSpawner::SpawnPlayer(const Protocol::ObjectInfo& InObjectInfo)
{
    UP1GameInstance* GameInstance = Cast<UP1GameInstance>(GetGameInstance());
    if (GameInstance == nullptr)
        return nullptr;

    UWorld* World = GetWorld();
    UP1MyPlayerData* MyPlayerData = GameInstance->GetMyPlayerData();
    uint64 MyPlayerId = MyPlayerData->GetPlayerId();
    bool IsMine = MyPlayerId == InObjectInfo.object_id();

    FVector SpawnLocation(InObjectInfo.pos_info().pos().x(), InObjectInfo.pos_info().pos().y(), InObjectInfo.pos_info().pos().z());
    FRotator SpawnRotation(0.f, InObjectInfo.pos_info().yaw(), 0.f);

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

        // 내 플레이어 Pawn은 GameInstance가 알고 있게한다.
        GameInstance->SetMyPlayer(Cast<AP1MyPlayer>(OutPlayer));
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
        FString PlayerName = InObjectInfo.player_info().name().c_str();
        OutPlayer->SetPlayerName(FText::FromString(PlayerName));
        OutPlayer->SetServerPos(InObjectInfo.pos_info());
        OutPlayer->FinishSpawning(FTransform(SpawnRotation, SpawnLocation));
        OutPlayer->Initialize(InObjectInfo);
    }
    else
    {
        UE_LOG(LogP1Entity, Warning, TEXT("플레이어 SpawnActorDeferred<> NullPtr 반환"));
    }

    return OutPlayer;
}


