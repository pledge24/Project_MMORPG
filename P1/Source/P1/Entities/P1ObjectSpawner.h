#pragma once

#include "CoreMinimal.h"
#include "Data/P1MonsterData.h"
#include "Characters/P1Monster.h"
#include "Characters/P1MyPlayer.h"
#include "GameFramework/Actor.h"
#include "P1ObjectSpawner.generated.h"

UCLASS()
class P1_API AP1ObjectSpawner : public AActor
{
    GENERATED_BODY()

public:
    AP1ObjectSpawner();

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

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
    TObjectPtr<UDataTable> MonsterDataTable;

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
