#pragma once
#include "Creature.h"
#include "Utils.h"

class TickIntervalTimer;

enum class MonsterState : uint8
{
    Idle = 0,
    Patrolling,
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
    virtual void PostConstructionSetup() override;
    virtual void Tick(float deltaTime) override;

public:
    void PostInit();
    void PrintMonsterAllData() const;

protected:
    void CacheMonsterData();

    /** 상태 함수 */
    void UpdateState();
    void SetState(MonsterState updatedState);

    void ExecuteStateBehavior(float deltaTime);
    void ExecuteStateNone();
    void ExecuteStateIdle(float deltaTime);
    void ExecuteStatePatrolling(float deltaTime);
    void ExecuteStateAttacking(float deltaTime);
    void ExecuteStateChasing(float deltaTime);
    void ExecuteStateDeath(float deltaTime);

    /** AI 함수 */
    void Move(float deltaTime);
    bool AlreadyArrive();

    void UpdateTargetPos();
    void LookAtTarget();

private:
    Protocol::MonsterInfo* monsterInfo;
    TickIntervalTimerRef stateIntervalTimer = nullptr;

    /** Monster Raw Data */
    Json _monsterData;

    /** Monster Stat Data */
    int32 templateId;
    int32 maxHp;
    float attackSpeed;
    int32 baseAttack;

    /** Monster AI Data(Common) */
    const float IDLE_TIME = 5.f;
    const float PATROL_MOVING_TIME = 2.f;
    const float UPDATE_STATE_INTERVAL = 1.f;
    const float MONSTER_SPEED = 300.f;  // TEST(cm per sec)

    /** Monster AI Data(Individual) */
    MonsterState state = MonsterState::Idle;
    shared_ptr<Protocol::PosInfo> spawnPos;
    float tryAttackRange;                       // 공격 사거리
    float detectionRange;                       // 타겟 감지 범위
    float chasingMaxRange;                      // 추적 범위

    weak_ptr<Object> _target;
    optional<vector2D> _targetPos;
    bool _shouldReturn = false;         // patrolling 할때 스폰 포인트로 이동 여부
    float _stateTimer = 0.f;            // 여러 용도로 사용됨
};

