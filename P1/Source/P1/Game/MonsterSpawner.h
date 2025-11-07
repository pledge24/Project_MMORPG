// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MonsterData.h"
#include "Monster.h"
#include "GameFramework/Actor.h"
#include "MonsterSpawner.generated.h"

UCLASS()
class P1_API AMonsterSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AMonsterSpawner();

protected:
	virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable, Category="Spawn")
    AMonster* SpawnMonster(int32 TemplateId, const FTransform& Transform);

    AMonster* SpawnMonster(int32 TemplateId, const FVector& Location, const FRotator& Rotation);

    //void LoadMonsterAsset();
    bool GetMonsterData(int32 TemplateId, FMonsterData& OutMonsterData);

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Data")
    UDataTable* MonsterDataTable;
};
