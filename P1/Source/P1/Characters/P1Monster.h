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

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;
    //~ End AActor Interface

    //~ Begin APawn Interface
public:
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    //~ End APawn Interface

    //~ Begin AP1Creature Interface
public:
    virtual void Initialize(const Protocol::ObjectInfo& ObjectInfo) override;
    //~ End AP1Creature Interface

    //~ Monster Data
public:
    const FP1MonsterData& GetMonsterData() const { return MonsterData; }
    int32 GetTemplateId() const { return TemplateId; }
    int32 GetCurHp() const { return CurHp; }

    void SetDefaultMonsterData(const FP1MonsterData& InMonsterData);

protected:
    /** 데이터 테이블에서 읽은 고정값이다. 런타임에 바뀌지 않는다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterData")
    FP1MonsterData MonsterData;

    /** MonsterData에서 꺼내 캐시한 값이다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterData")
    int32 TemplateId = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterData")
    int32 CurHp = -1;

    Protocol::MonsterInfo _MonsterInfo;
};
