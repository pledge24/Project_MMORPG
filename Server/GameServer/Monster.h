#pragma once
#include "Creature.h"
#include "Utils.h"

class TickIntervalTimer;

enum class MonsterState : uint8
{
    Idle = 0,
    Wandering,
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

    virtual void OnHit(ObjectRef attacker, Protocol::HitData& hitData) override;

protected:
    void CacheMonsterData();

    /** 상태 함수 */
    void UpdateState();
    void ChangeState(MonsterState changedState);

    void ExecuteStateBehavior(float deltaTime);
    void ExecuteStateNone();
    void ExecuteStateIdle(float deltaTime);
    void ExecuteStateWandering(float deltaTime);
    void ExecuteStateAttacking(float deltaTime);
    void ExecuteStateChasing(float deltaTime);
    void ExecuteStateDeath(float deltaTime);

    /** AI 함수 */
    void Move(float deltaTime, bool orientRotationToMovement = true);
    void LookAt(const vector2D& targetPos);
    void StartMovingTo(const vector2D& dest, float minApproachDistance = 0.f);
    void StopMoving(string context = "", bool shouldBeIdle = false);
    void Attack();  // normal attack

    bool CanMove();
    bool AlreadyArrive();
    bool HaveDestination() { return _moveDest.has_value(); }

    /** 이벤트 */
    void OnHitCheck();

    /** Getter-Setter 함수 */
    const vector2D& GetDestination() { return _moveDest.value(); }
    uint64 GetExpReward();
    uint64 GetGoldReward();

    void SetDestination(const vector2D& destPos, float minApproachDistance = 0.f);
    void SetMoveDirection(const vector2D& moveVec);
    void SetYaw(float yaw);

    void ClearDestination();

    /** 네트워크 함수 */
    void ForceBroadcastMovePkt();

private:
    Protocol::MonsterInfo* monsterInfo;
    TickIntervalTimerRef stateIntervalTimer = nullptr;

    /** Monster Raw Data */
    Json _monsterData;

    /** Monster Stat Data */
    int32 templateId;
    int32 maxHp;
    float attackInterval;
    int32 baseAttack;

    /** Monster AI Data(Common) */
    const float IDLE_TIME = 5.f;
    const float WANDERING_TIME = 2.f;
    const float UPDATE_STATE_INTERVAL = 1.f;
    const float MIN_APPROACH_DISTANCE = 120.f;

    /** Monster AI Data(Individual) */
    MonsterState state = MonsterState::Idle;
    vector2D spawnPos;
    float tryAttackRange;                       // 공격 사거리
    float detectionRange;                       // 타겟 감지 범위
    float chasingMaxRange;                      // 추적 범위
    float monsterSpeed;                         // 몬스터 이동 속도
    bool isTargeting;

    weak_ptr<Object> _target;
    optional<vector2D> _moveDest;
    float _stateTimer = 0.f;                    // 여러 용도로 사용됨
    float _timeSinceLastAttack = 0.f;
};

