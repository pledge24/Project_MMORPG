#pragma once
#include "Game/Entities/EntityComponent.h"
#include "Game/Combat/CombatTypes.h"
#include "Game/AI/MonsterStates.h"

/**
 * 몬스터 하나의 행동을 정하는 두뇌.
 *
 * 대기, 배회, 추적, 공격, 귀환 상태 사이를 오가며 어디로 갈지와 언제 공격할지를 정한다. 룸 큐 위에서만 쓴다.
 */
class MonsterAIComponent : public EntityComponent
{
public:
    MonsterAIComponent(MonsterRef owner, const DesignData::MonsterRow& monsterRow, const CreatureCombat& combat);
    virtual ~MonsterAIComponent() = default;

    //~ Begin EntityComponent Interface
public:
    virtual void Tick(float deltaTime) override;
    //~ End EntityComponent Interface

    //~ 상태 기계
public:
    MonsterStateType GetState() const { return _state->GetType(); }
    /** 쫓거나 공격하는 대상. 없으면 nullptr. */
    EntityRef GetTarget() const { return _target.lock(); }

private:
    /** 지금 상태를 떠나고 nextState에 들어간다. */
    void SwitchState(MonsterStateType nextState);
    /** 대상이 사라졌거나, 사망했거나, 이 룸을 떠났으면 true. */
    bool IsTargetLost() const;

    /** 상태 갱신의 주기(초). */
    static constexpr float UPDATE_STATE_INTERVAL = 0.2f;

    const MonsterState* _state = nullptr;
    weak_ptr<Entity> _target;
    /** 지금 상태에 들어온 뒤 흐른 시간(초). */
    float _stateTimer = 0.f;
    /** 마지막 상태 갱신 뒤 흐른 시간(초). */
    float _timeSinceStateUpdate = 0.f;

    //~ 상태가 쓰는 창구
public:
    /** 지금 상태에 들어온 뒤 흐른 시간(초). */
    float GetStateTime() const { return _stateTimer; }

    /** 감지 범위 안에서 가장 가까운 살아 있는 플레이어를 대상으로 잡는다. 없으면 false. */
    bool FindTarget();
    void ClearTarget() { _target.reset(); }
    /** 아래 대상 함수는 대상이 있고 이 룸에 있을 때만 부른다. AI가 틱마다 대상을 놓쳤는지 먼저 본다. */
    bool IsTargetInAttackRange() const;
    void LookAtTarget();

    /** 스폰 지점에서 잰 평면 거리가 추적 한계를 넘었으면 true. */
    bool IsBeyondChaseLimit() const;

    /** 대상 앞까지 길을 찾아 움직인다. 공격한 뒤 공격 간격 동안은 움직이지 않는다. */
    void ChaseTarget();
    /** 스폰 반경 안의 무작위 지점으로 움직인다. */
    void WanderInSpawnRadius();
    /** 스폰 반경 안에 들 때까지 스폰 지점으로 움직인다. */
    void ReturnToSpawnPoint();
    bool IsMoving() const;
    /** 멈추고 이동 상태를 Idle로 둔다. */
    void StopMoving();
    /** 멈추고 대상을 바라보며 이동 상태를 Action으로 둔다. */
    void HoldForAttack();

    /** 마지막 공격 뒤 공격 간격이 지났으면 true. */
    bool CanAttack() const { return _timeSinceLastAttack >= _attackInterval; }
    /** 룸에 일반 공격을 알리고, 공격 몽타주의 타격 시점 뒤의 피격 판정을 예약한다. */
    void NormalAttack();
    /** 예약한 피격 판정을 버린다. */
    void CancelPendingHit() { _pendingHit.reset(); }

    /** deltaTime(초)만큼 귀환 중의 체력을 회복한다. 최대 HP를 넘지 않는다. */
    void RecoverHp(float deltaTime);

private:
    /** 귀환하는 동안 초당 회복하는 체력. 최대 HP에 대한 비율이다. */
    static constexpr float RETURN_HP_RECOVERY_RATIO = 0.2f;
    /** 추적은 대상에서 이만큼 떨어진 곳에서 멈춘다(cm). */
    static constexpr float MIN_APPROACH_DISTANCE = 120.f;

    float _detectionRange;
    /** 스폰 지점에서 잰다(cm). */
    float _chasingMaxRange;
    /** 공격 사거리(cm). */
    float _tryAttackRange;

    //~ 공격
private:
    /** 예약한 피격의 때가 되면 룸에 판정을 넘긴다. */
    void UpdatePendingHit(float deltaTime);

    /** 공격 간격(초). */
    float _attackInterval;
    int32 _baseAttack;
    /** 일반 공격을 알린 뒤 피격을 판정하기까지의 시간(초). 공격 몽타주의 타격 시점이다. */
    float _normalAttackHitDelay;

    /** 마지막 공격 뒤 흐른 시간(초). 스폰한 몬스터는 공격한 적이 없으므로 공격 간격에서 시작한다. */
    float _timeSinceLastAttack;

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
    /** 대상의 위치. 대상이 없으면 nullptr. */
    const Protocol::PosInfo* GetTargetPos() const;
};
