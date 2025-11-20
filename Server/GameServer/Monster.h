#pragma once
#include "Creature.h"
#include "Utils.h"

class TickTimer;

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
    void Init();
    void PrintMonsterAllData() const;

protected:
    void CacheMonsterData();

    /** 상태 함수 */
    void ProcessNone();
    void ProcessIdle(float deltaTime);
    void ProcessPatrolling(float deltaTime);
    void ProcessAttacking(float deltaTime);
    void ProcessChasing(float deltaTime);
    void ProcessDeath(float deltaTime);

    void SetState(MonsterState updatedState);

    /** AI 함수 */
    void Move(float deltaTime);
    bool AlreadyArrive();

private:
    Protocol::MonsterInfo* monsterInfo;
    TickTimerRef stateTickTimer = nullptr;

    /** Monster Raw Data */
    Json _monsterData;

    /** Monster Stat Data */
    int32 templateId;
    int32 maxHp;
    float attackSpeed;
    int32 baseAttack;

    /** Monster AI Data(Individual) */
    MonsterState state = MonsterState::Idle;
    shared_ptr<Protocol::PosInfo> spawnPos;
    float attackRange;                  // 공격 사거리
    float detectionRange;               // 타겟 감지 범위
    float chaseRange;                   // 추적 범위

    /** Monster AI Data(Common) */
    const float STANDING_TIME = 3.f;
    const float PATROL_MOVING_TIME = 5.f;
    const float MONSTER_SPEED = 500.f;  // TEST(cm per sec)

    vector2D targetPos;
    bool shouldReturn = false;          // patrolling 할때 스폰 포인트로 이동 여부
};

