#pragma once

#include "CoreMinimal.h"
#include "Game/Data/P1MonsterData.h"
#include "Game/Entities/P1Monster.h"
#include "Game/Entities/P1MyPlayer.h"
#include "GameFramework/Actor.h"
#include "P1EntitySpawner.generated.h"

UCLASS()
class P1_API AP1EntitySpawner : public AActor
{
    GENERATED_BODY()

public:
    AP1EntitySpawner();

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    //~ End AActor Interface

    //~ Monster Spawn
public:
    UFUNCTION(BlueprintCallable, Category = "Spawn")
    AActor* SpawnMonster(int32 TemplateId, const FTransform& Transform);

    /** 서버가 보낸 엔티티 정보로 스폰한다. 서버가 보낸 값으로만 부른다. */
    AActor* SpawnMonster(const Protocol::EntityInfo& InEntityInfo);

    AActor* SpawnMonster(int32 TemplateId, const FVector& SpawnLocation, const FRotator& SpawnRotation, TOptional<Protocol::EntityInfo> ServerInfo = NullOpt);

    bool GetMonsterData(int32 TemplateId, FP1MonsterData& OutMonsterData);

    /** 몬스터 클래스를 에셋 테이블에서 찾아 로드한다. 행이 없으면 nullptr */
    TSubclassOf<AP1Monster> GetMonsterClass(int32 TemplateId) const;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
    TObjectPtr<UDataTable> MonsterDataTable;

    /** 몬스터 클래스를 담은 에셋 테이블이다. 행 구조체는 FP1MonsterAssetData다. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
    TObjectPtr<UDataTable> MonsterAssetTable;

    //~ Player Spawn
public:
    /** 서버가 보낸 엔티티 정보로 스폰한다. 서버가 보낸 값으로만 부른다. */
    AActor* SpawnPlayer(const Protocol::EntityInfo& InEntityInfo);

protected:
    UPROPERTY(EditAnywhere)
    TSubclassOf<AP1MyPlayer> MyPlayerClass;

    UPROPERTY(EditAnywhere)
    TSubclassOf<AP1Player> OtherPlayerClass;
};
