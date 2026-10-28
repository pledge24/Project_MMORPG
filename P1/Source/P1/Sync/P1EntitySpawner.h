#pragma once

#include "CoreMinimal.h"
#include "Game/Entities/P1Monster.h"
#include "Game/Entities/P1MyPlayer.h"
#include "GameFramework/Actor.h"
#include "P1EntitySpawner.generated.h"

/**
 * 엔티티 액터를 만드는 스포너.
 *
 * 레벨에 배치되어, 서버가 알려 준 몬스터와 플레이어를 월드의 액터로 만든다.
 */
UCLASS()
class P1_API AP1EntitySpawner : public AActor
{
    GENERATED_BODY()

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    //~ End AActor Interface

    //~ Monster Spawn
public:
    /** 서버 정보 없이 템플릿 id로 몬스터를 스폰한다. 실패하면 nullptr. */
    UFUNCTION(BlueprintCallable, Category = "Spawn")
    AActor* SpawnMonster(int32 TemplateId, const FTransform& Transform);

    /** 서버가 보낸 엔티티 정보로 스폰한다. 실패하면 nullptr. */
    AActor* SpawnMonster(const Protocol::EntityInfo& InEntityInfo);

    /** ServerInfo가 있으면 그 정보로 위치와 엔티티를 초기화한다. 실패하면 nullptr. */
    AActor* SpawnMonster(int32 TemplateId, const FVector& SpawnLocation, const FRotator& SpawnRotation, TOptional<Protocol::EntityInfo> ServerInfo = NullOpt);

protected:
    /** 모든 몬스터를 스폰할 공용 클래스. 종류마다 다른 외형은 몬스터 에셋 테이블의 행이 정한다. */
    UPROPERTY(EditAnywhere)
    TSubclassOf<AP1Monster> MonsterClass;

    //~ Player Spawn
public:
    /**
     * 서버가 보낸 엔티티 정보로 스폰한다. 실패하면 nullptr.
     * 내 플레이어면 초기화를 마친 뒤 OnMyPlayerSpawned를 알린다.
     */
    AActor* SpawnPlayer(const Protocol::EntityInfo& InEntityInfo);

protected:
    /** 내 플레이어로 스폰할 클래스. */
    UPROPERTY(EditAnywhere)
    TSubclassOf<AP1MyPlayer> MyPlayerClass;

    /** 다른 플레이어로 스폰할 클래스. */
    UPROPERTY(EditAnywhere)
    TSubclassOf<AP1Player> OtherPlayerClass;
};
