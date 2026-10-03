#pragma once

#include "CoreMinimal.h"
#include "Game/Data/P1MonsterData.h"
#include "Game/Entities/P1Creature.h"
#include "P1Monster.generated.h"

class AP1Monster;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDespawnReady, AP1Monster*);

UCLASS()
class P1_API AP1Monster : public AP1Creature
{
    GENERATED_BODY()

    //~ Begin AP1Creature Interface
public:
    virtual void Initialize(const Protocol::EntityInfo& EntityInfo) override;
    /** 사망 연출 뒤 OnDespawnReady를 알린다. 서버는 사망한 몬스터에 S_DESPAWN을 보내지 않는다. */
    virtual void S_Die() override;
    //~ End AP1Creature Interface

    //~ Death
public:
    /** 사망 연출이 끝나 지워도 될 때 한 번 알린다. 지우는 일은 구독한 쪽이 한다. */
    FOnDespawnReady OnDespawnReady;

private:
    void DespawnAfterDeath();

    /** 초 단위. 사망 애니메이션(약 2.1초)이 끝나고 잠시 뒤에 지운다. */
    static constexpr float DESPAWN_DELAY_SECONDS = 3.0f;

    FTimerHandle DespawnTimerHandle;

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
