#pragma once

#include "CoreMinimal.h"
#include "Game/Data/Generated/P1MonsterRow.h"
#include "Game/Entities/P1Creature.h"
#include "P1Monster.generated.h"

class AP1Monster;
struct FP1MonsterAssetData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDespawnReady, AP1Monster*);

/**
 * 월드에 보이는 몬스터.
 *
 * 모든 몬스터 종류가 이 클래스 하나를 쓰고, 종류마다 다른 외형은 표현 데이터로 입는다.
 * 몬스터 종류의 고정 데이터를 들고, 사망한 몬스터를 연출 뒤 정리할 수 있게 알린다.
 */
UCLASS()
class P1_API AP1Monster : public AP1Creature
{
    GENERATED_BODY()

public:
    AP1Monster();

    //~ Begin AP1Creature Interface
public:
    virtual void Initialize(const Protocol::EntityInfo& EntityInfo) override;
    /** 사망 연출 뒤 OnDespawnReady를 알린다. */
    virtual void S_Die() override;
    //~ End AP1Creature Interface

    //~ Death
public:
    /** 사망 연출이 끝나 지워도 될 때 한 번 알린다. 지우는 일은 구독한 쪽이 한다. */
    FOnDespawnReady OnDespawnReady;

private:
    void DespawnAfterDeath();

    /** 사망부터 지우기까지의 시간(초). 사망 애니메이션(약 2.1초)이 끝나고 잠시 뒤다. */
    static constexpr float DESPAWN_DELAY_SECONDS = 3.0f;

    FTimerHandle DespawnTimerHandle;

    //~ Appearance
public:
    /**
     * 표현 데이터의 메시, 애님 인스턴스, 머티리얼, 크기, 캡슐, 공격 몽타주를 입힌다. 가리키는 에셋을 동기 로드한다.
     * FinishSpawning 전에 불러야 BeginPlay의 컴포넌트들이 바뀐 메시를 본다.
     */
    void ApplyAssetData(const FP1MonsterAssetData& AssetData);

    //~ Monster Data
public:
    const FP1MonsterRow& GetMonsterData() const { return MonsterData; }
    int32 GetTemplateId() const { return TemplateId; }
    int32 GetCurHp() const { return CurHp; }

    void SetDefaultMonsterData(const FP1MonsterRow& InMonsterData);

protected:
    /** 데이터 테이블에서 읽은 고정값. 런타임에 바뀌지 않는다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterData")
    FP1MonsterRow MonsterData;

    /** 몬스터 종류를 가리키는 템플릿 ID. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterData")
    int32 TemplateId = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterData")
    int32 CurHp = -1;

    Protocol::MonsterInfo _MonsterInfo;
};
