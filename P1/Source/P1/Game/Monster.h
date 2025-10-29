// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MonsterData.h"
#include "Monster.generated.h"

UCLASS()
class P1_API AMonster : public ACharacter
{
	GENERATED_BODY()

public:
	AMonster();

protected:
	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    const FMonsterData& GetMonsterData() const { return MonsterData; }
    int64 GetCurHp() const { return CurHp; }

protected:
    /** 고정된 데이터 */
    FMonsterData MonsterData;

    int64 CurHp = -1;
};
