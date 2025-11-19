#pragma once
#include "Creature.h"

class TickTimer;

enum class MonsterState : uint8
{
    Idle = 0,
    Chasing,
    Attacking,
    Death,
    StateCount
};

class Monster : public Creature
{
public:
	Monster();
	virtual ~Monster();

protected:
    virtual void Tick(float deltaSecond) override;

public:
    void Init();
    void PrintMonsterAllData() const;

private:
    void CacheMonsterData();

    /** 상태 함수 */
    void ProcessNone();
    void ProcessIdle(float deltaSecond);
    void ProcessAttacking(float deltaSecond);
    void ProcessChasing(float deltaSecond);
    void ProcessDeath(float deltaSecond);

    void SetState(MonsterState updatedState);

private:
    Protocol::MonsterInfo* monsterInfo;
    TickTimerRef StateTickTimer = nullptr;

    /** Monster Raw Data */
    Json _monsterData;

    /** Monster Stat Data */
    int32 templateId;
    int32 maxHp;
    float attackSpeed;
    int32 baseAttack;

    /** Monster AI Data(Individual) */
    MonsterState state = MonsterState::Idle;
    float attackRange;                  // 공격 사거리
    float detectionRange;               // 타겟 감지 범위
    float chaseRange;                   // 추적 범위

    /** Monster AI Data(Common) */
    const float IDLE_MOVING_TIME = 3.f;
    const float IDLE_STANDING_TIME = 5.f;

};

