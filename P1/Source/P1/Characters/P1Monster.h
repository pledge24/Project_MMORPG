#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/P1MonsterData.h"
#include "Characters/P1Creature.h"
#include "P1Monster.generated.h"

UCLASS()
class P1_API AP1Monster : public AP1Creature
{
	GENERATED_BODY()

public:
	AP1Monster();

protected:
	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

public:	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void Initialize(const Protocol::ObjectInfo& ObjectInfo) override;

    const FP1MonsterData& GetMonsterData() const { return MonsterData; }
    int32 GetTemplateId() const { return TemplateId; }
    int32 GetCurHp() const { return CurHp; }

    void SetDefaultMonsterData(const FP1MonsterData& InMonsterData);

protected:
    /** 고정된 데이터 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MonsterData")
    FP1MonsterData MonsterData;

    // Cached
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MonsterData")
    int32 TemplateId = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MonsterData")
    int32 CurHp = -1;

    Protocol::MonsterInfo _MonsterInfo;

};
