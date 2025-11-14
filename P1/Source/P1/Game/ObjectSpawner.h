// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MonsterData.h"
#include "Monster.h"
#include "P1MyPlayer.h"
#include "GameFramework/Actor.h"
#include "ObjectSpawner.generated.h"

UCLASS()
class P1_API AObjectSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AObjectSpawner();

protected:
	virtual void BeginPlay() override;

public:
    /** Monster 관련 함수 */
    UFUNCTION(BlueprintCallable, Category="Spawn")
    bool SpawnMonster(int32 TemplateId, AActor* OutMonster, const FTransform& Transform);

    bool SpawnMonster(AMonster* OutMonster, const Protocol::ObjectInfo& InObjectInfo); // Server Only
    bool SpawnMonster(int32 TemplateId, AMonster* OutMonster, const FVector& SpawnLocation, const FRotator& SpawnRotation, TOptional<Protocol::ObjectInfo> ServerInfo = NullOpt);

    bool GetMonsterData(int32 TemplateId, FMonsterData& OutMonsterData);

    /** Player 관련 함수 */
    bool SpawnPlayer(AP1Player* OutPlayer, const Protocol::ObjectInfo& InObjectInfo); // Server Only

protected:
    /** Monster 정보 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Data")
    UDataTable* MonsterDataTable;

    /** Player 정보 */
    UPROPERTY(EditAnywhere)
    TSubclassOf<AP1MyPlayer> MyPlayerClass;

    UPROPERTY(EditAnywhere)
    TSubclassOf<AP1Player> OtherPlayerClass;

};
