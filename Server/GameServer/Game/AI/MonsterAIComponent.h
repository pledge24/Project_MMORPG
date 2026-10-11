#pragma once
#include "Game/Entities/EntityComponent.h"

/** 몬스터 AI의 상태. 사망한 몬스터는 룸에서 빠지므로 사망 상태를 두지 않는다. */
enum class MonsterState : uint8
{
    Idle = 0,
    Wandering,
    Chasing,
    Attacking,
};

/**
 * 몬스터 하나의 행동을 정하는 컴포넌트. 배회, 추적, 공격의 상태 기계와 이동을 맡는다.
 * 소유 몬스터의 Tick에서 룸 큐 위로 불린다. 이동과 행동은 틱마다, 상태 전환 판정은 누적 시간이
 * UPDATE_STATE_INTERVAL을 채울 때마다 돈다. 위치는 소유 몬스터의 함수로만 바꾼다.
 */
class MonsterAIComponent : public EntityComponent
{
public:
    /** 몬스터 표의 AI 값을 읽어 두고 Idle 상태로 시작한다. 배회 목적지는 룸 안의 무작위 위치다. */
    MonsterAIComponent(MonsterRef owner, const MonsterTemplate& monsterTemplate);
    virtual ~MonsterAIComponent() = default;

    //~ Begin EntityComponent Interface
    /** 예약한 피격, 상태 전환 판정(주기가 찼을 때), 상태별 행동 순서로 돈다. */
    virtual void Tick(float deltaTime) override;
    //~ End EntityComponent Interface

    //~ 상태 읽기
    MonsterState GetState() const { return _state; }
    /** 쫓거나 공격하는 대상. 없으면 nullptr. */
    EntityRef GetTarget() const { return _target.lock(); }

private:
    /** 소유 몬스터. 소유자가 이미 사라졌으면 nullptr. */
    MonsterRef GetOwner() const;

    //~ 상태 전환
    void EvaluateStateTransition();
    /** _stateTimer를 0으로 돌리고 새 상태의 진입 처리(목적지, 방향, 이동 상태)를 한다. */
    void SwitchState(MonsterState nextState);

    /** 상태별 행동을 틱마다 실행한다. deltaTime은 초 단위다. */
    void ExecuteStateBehavior(float deltaTime);
    void ExecuteStateWandering(float deltaTime);
    void ExecuteStateAttacking(float deltaTime);
    void ExecuteStateChasing(float deltaTime);

    //~ 이동
    /** 목적지가 없거나 이미 도착했으면 이동하지 않고 멈춘다. deltaTime은 초 단위다. */
    void Move(float deltaTime, bool orientRotationToMovement = true);
    void LookAt(const vector2D& targetPos);
    void StartMovingTo(const vector2D& dest, float minApproachDistance = 0.f);
    /** shouldBeIdle이면 달리던 중이 아니어도 Idle 이동 상태로 바꾼다. */
    void StopMoving(bool shouldBeIdle = false);
    /** minApproachDistance가 0보다 크면 목적지에서 그만큼 덜 간 지점을 목적지로 잡는다. 이미 그보다 가까우면 현재 위치다. */
    void SetDestination(const vector2D& destPos, float minApproachDistance = 0.f);
    void ClearDestination();

    //~ 공격
    /** 룸에 일반 공격을 알리고, NORMAL_ATTACK_HIT_DELAY 뒤의 피격 판정을 예약한다. */
    void NormalAttack();
    /**
     * 예약한 피격의 남은 시간을 줄이고, 다 됐으면 Room::HandleHit에 넘긴다.
     * 그때 대상이 이 룸에 없거나 사거리(_tryAttackRange) 밖이면 피격을 버린다.
     */
    void UpdatePendingHit(float deltaTime);

    //~ 상태 확인
    /** 대상이 사라졌거나, 사망했거나, 이 룸을 떠났으면 true. */
    bool IsTargetLost();
    /** 목적지가 있고 마지막 공격 뒤 _attackInterval이 지났을 때만 true. */
    bool CanMove();
    /** 목적지가 없어도 true다. */
    bool AlreadyArrive();

private:
    //~ 공통 값
    /** 단위는 초다. */
    static constexpr float IDLE_TIME = 5.f;
    /** 단위는 초다. */
    static constexpr float WANDERING_TIME = 2.f;
    /** 상태 전환 판정의 주기. 단위는 초다. */
    static constexpr float UPDATE_STATE_INTERVAL = 0.2f;
    /** 일반 공격을 알린 뒤 피격을 판정하기까지의 시간. 단위는 초다. */
    static constexpr float NORMAL_ATTACK_HIT_DELAY = 0.2f;
    static constexpr float MIN_APPROACH_DISTANCE = 120.f;

    //~ 몬스터 표에서 읽은 값
    /** 공격 간격(초). */
    float _attackInterval;
    int32 _baseAttack;
    float _tryAttackRange;                      // 공격 사거리
    float _detectionRange;                      // 타겟 감지 범위
    float _chasingMaxRange;                     // 추적 범위
    float _monsterSpeed;                        // 몬스터 이동 속도

    //~ 상태 기계의 값
    MonsterState _state = MonsterState::Idle;
    weak_ptr<Entity> _target;
    optional<vector2D> _moveDest;

    //~ 타이머
    /** 아래 세 타이머의 단위는 초다. */
    float _stateTimer = 0.f;                    // 여러 용도로 사용됨
    float _timeSinceLastAttack = 0.f;
    float _timeSinceStateUpdate = 0.f;

    /** 판정을 기다리는 일반 공격. remainingTime은 초 단위다. */
    struct PendingHit
    {
        Protocol::AttackInfo attackInfo;
        /** 공격을 알릴 때의 대상. 그사이 _target이 바뀌어도 이 대상으로 판정한다. */
        weak_ptr<Entity> target;
        float remainingTime = 0.f;
    };
    optional<PendingHit> _pendingHit;
};
