#pragma once
#include "Game/Entities/EntityComponent.h"
#include "Game/Combat/CombatTypes.h"

/** 몬스터 AI의 상태. 사망한 몬스터는 룸에서 빠지므로 사망 상태를 두지 않는다. */
enum class MonsterState : uint8
{
    Idle = 0,
    Wandering,
    Chasing,
    Attacking,
};

/**
 * 몬스터 하나의 행동을 정하는 두뇌.
 *
 * 대기, 배회, 추적, 공격 사이를 오가며 몬스터를 움직이고 공격하게 한다.
 */
class MonsterAIComponent : public EntityComponent
{
public:
    MonsterAIComponent(MonsterRef owner, const MonsterTemplate& monsterTemplate);
    virtual ~MonsterAIComponent() = default;

    //~ Begin EntityComponent Interface
public:
    virtual void Tick(float deltaTime) override;
    //~ End EntityComponent Interface

    //~ 상태 기계
public:
    MonsterState GetState() const { return _state; }
    /** 쫓거나 공격하는 대상. 없으면 nullptr. */
    EntityRef GetTarget() const { return _target.lock(); }

private:
    void EvaluateStateTransition();
    /** 상태를 바꾸고, 그 상태에 맞게 목적지와 이동 상태를 맞춘다. */
    void SwitchState(MonsterState nextState);

    /** 현재 상태의 행동을 한 틱만큼 실행한다. deltaTime은 초 단위다. */
    void ExecuteStateBehavior(float deltaTime);
    void ExecuteStateWandering(float deltaTime);
    void ExecuteStateAttacking(float deltaTime);
    void ExecuteStateChasing(float deltaTime);

    /** 대상이 사라졌거나, 사망했거나, 이 룸을 떠났으면 true. */
    bool IsTargetLost();

    /** 대기를 유지하는 시간(초). */
    static constexpr float IDLE_TIME = 5.f;
    /** 배회를 유지하는 최대 시간(초). */
    static constexpr float WANDERING_TIME = 2.f;
    /** 상태 전환 판정의 주기(초). */
    static constexpr float UPDATE_STATE_INTERVAL = 0.2f;
    static constexpr float MIN_APPROACH_DISTANCE = 120.f;

    float _detectionRange;
    float _chasingMaxRange;

    MonsterState _state = MonsterState::Idle;
    weak_ptr<Entity> _target;

    /** 지금 상태에 들어온 뒤 흐른 시간(초). */
    float _stateTimer = 0.f;
    /** 마지막 상태 전환 판정 뒤 흐른 시간(초). */
    float _timeSinceStateUpdate = 0.f;

    //~ 이동
private:
    /** 목적지로 한 틱만큼 이동한다. 목적지가 없거나 이미 도착했으면 멈춘다. deltaTime은 초 단위다. */
    void Move(float deltaTime, bool orientRotationToMovement = true);
    void LookAt(const vector2D& targetPos);
    void StartMovingTo(const vector2D& dest, float minApproachDistance = 0.f);
    /** shouldBeIdle이면 달리던 중이 아니어도 Idle 이동 상태로 바꾼다. */
    void StopMoving(bool shouldBeIdle = false);
    /** minApproachDistance가 0보다 크면 목적지에서 그만큼 덜 간 지점을 목적지로 잡는다. 이미 그보다 가까우면 현재 위치다. */
    void SetDestination(const vector2D& destPos, float minApproachDistance = 0.f);
    void ClearDestination();

    /** 목적지가 있고 마지막 공격 뒤 _attackInterval이 지났을 때만 true. */
    bool CanMove();
    /** 목적지가 없어도 true다. */
    bool AlreadyArrive();

    float _monsterSpeed;
    optional<vector2D> _moveDest;

    //~ 공격
private:
    /** 룸에 일반 공격을 알리고, 잠시 뒤의 피격 판정을 예약한다. */
    void NormalAttack();
    /** 예약한 피격의 때가 되면 룸에 판정을 넘긴다. */
    void UpdatePendingHit(float deltaTime);

    /** 일반 공격을 알린 뒤 피격을 판정하기까지의 시간(초). */
    static constexpr float NORMAL_ATTACK_HIT_DELAY = 0.2f;

    /** 공격 간격(초). */
    float _attackInterval;
    int32 _baseAttack;
    /** 공격 사거리. */
    float _tryAttackRange;

    /** 마지막 공격 뒤 흐른 시간(초). */
    float _timeSinceLastAttack = 0.f;

    /** 판정을 기다리는 일반 공격. remainingTime은 초 단위다. */
    struct PendingHit
    {
        AttackInfo attackInfo;
        /** 공격을 알릴 때의 대상. 그사이 _target이 바뀌어도 이 대상으로 판정한다. */
        weak_ptr<Entity> target;
        float remainingTime = 0.f;
    };
    optional<PendingHit> _pendingHit;

    //~ 공통
private:
    /** 소유 몬스터. 소유자가 이미 사라졌으면 nullptr. */
    MonsterRef GetOwner() const;
};
