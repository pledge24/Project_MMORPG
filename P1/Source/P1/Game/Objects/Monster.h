// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MonsterData.h"
#include "Objects/Creature.h"
#include "Monster.generated.h"

UCLASS()
class P1_API AMonster : public ACreature
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

    virtual void Initialize(const Protocol::ObjectInfo& ObjectInfo) override;

    const FMonsterData& GetMonsterData() const { return MonsterData; }
    int32 GetTemplateId() const { return TemplateId; }
    int32 GetCurHp() const { return CurHp; }

    void SetDefaultMonsterData(const FMonsterData& InMonsterData);

protected:
    /** 고정된 데이터 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MonsterData")
    FMonsterData MonsterData;

    // Cached
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MonsterData")
    int32 TemplateId = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MonsterData")
    int32 CurHp = -1;

    Protocol::MonsterInfo _MonsterInfo;

};
