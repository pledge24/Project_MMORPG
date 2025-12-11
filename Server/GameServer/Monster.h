#pragma once
#include "Creature.h"
#include "Utils.h"

class TickIntervalTimer;
class TickTimer;

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

public:
    virtual bool Init(Protocol::PosInfo* spawnPos = nullptr) override;
    virtual bool Start() override;

protected:
    virtual void Tick(float deltaTime) override;

public:
    /** 이벤트 함수 */
    virtual void OnHit(ObjectRef attacker, Protocol::AttackInfo attackInfo) override;
    virtual void OnDie(ObjectRef attacker) override;

    /** Getter 함수 */
    int64 GetTemplateId() { return templateId; }
    int64 GetExpReward();
    int64 GetGoldReward();

protected:
    /** 상태 함수 */
    void UpdateState();
    void SwitchState(MonsterState nextState);

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
    void NormalAttack();

    /** Bool 함수 */
    bool CanMove();
    bool AlreadyArrive();
    bool HasDestination() { return _moveDest.has_value(); }
    bool IsTargetingAttack(Protocol::AttackType type);

    /** Getter 함수(Private) */
    const vector2D& GetDestination() { return _moveDest.value(); }
 
    /** Setter 함수(Private) */
    void SetDestination(const vector2D& destPos, float minApproachDistance = 0.f);
    void SetMoveDirection(const vector2D& moveVec);
    void SetYaw(float yaw);

    /** 기타 함수 */
    void ClearDestination();
    void PrintMonsterAllData() const;
    void CacheMonsterData();

private:
    /** Monster Raw Data */
    Json _monsterData;

    /** Monster Stat Data */
    Protocol::MonsterInfo* monsterInfo;
    int32 templateId;
    int32 maxHp;
    float attackInterval;
    int32 baseAttack;

    /** Monster AI Data(Common) */
    const float IDLE_TIME = 5.f;
    const float WANDERING_TIME = 2.f;
    const uint64 UPDATE_STATE_INTERVAL_MS = 200;
    const float MIN_APPROACH_DISTANCE = 120.f;

    /** Monster AI Data(Individual) */
    MonsterState state = MonsterState::Idle;
    vector2D _spawnPos;
    float tryAttackRange;                       // 공격 사거리
    float detectionRange;                       // 타겟 감지 범위
    float chasingMaxRange;                      // 추적 범위
    float monsterSpeed;                         // 몬스터 이동 속도
    bool isTargeting;

    weak_ptr<Object> _target;
    optional<vector2D> _moveDest;

    /** Timer */
    float _stateTimer = 0.f;                    // 여러 용도로 사용됨
    float _timeSinceLastAttack = 0.f;

    TickTimer* attackTimer = nullptr;           // 사용 안하는 중
};

