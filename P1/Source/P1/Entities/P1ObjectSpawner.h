// Fill out your copyright notice in the Description page of Project Settings.

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

protected:
	virtual void BeginPlay() override;

public:
    /** Monster 관련 함수 */
    UFUNCTION(BlueprintCallable, Category="Spawn")
    AActor* SpawnMonster(int32 TemplateId, const FTransform& Transform);

    AActor* SpawnMonster(const Protocol::ObjectInfo& InObjectInfo); // Server Only
    AActor* SpawnMonster(int32 TemplateId, const FVector& SpawnLocation, const FRotator& SpawnRotation, TOptional<Protocol::ObjectInfo> ServerInfo = NullOpt);

    bool GetMonsterData(int32 TemplateId, FP1MonsterData& OutMonsterData);

    /** Player 관련 함수 */
    AActor* SpawnPlayer(const Protocol::ObjectInfo& InObjectInfo); // Server Only

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
