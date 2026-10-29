#include "Core/pch.h"
#include "Game/AI/MonsterStates.h"
#include "Game/AI/MonsterAIComponent.h"

namespace
{
    /** 대상을 찾았으면 공격 사거리 안이면 공격, 밖이면 추적. 못 찾았으면 nullopt. */
    optional<MonsterStateType> EngageIfTargetFound(MonsterAIComponent& ai)
    {
        if (ai.FindTarget() == false)
            return nullopt;

        return ai.IsTargetInAttackRange() ? MonsterStateType::Attacking : MonsterStateType::Chasing;
    }
}

const MonsterState& MonsterState::Get(MonsterStateType type)
{
    static const MonsterIdleState idle;
    static const MonsterWanderingState wandering;
    static const MonsterChasingState chasing;
    static const MonsterAttackingState attacking;
    static const MonsterReturningState returning;

    switch (type)
    {
    case MonsterStateType::Wandering:
        return wandering;
    case MonsterStateType::Chasing:
        return chasing;
    case MonsterStateType::Attacking:
        return attacking;
    case MonsterStateType::Returning:
        return returning;
    default:
        return idle;
    }
}

void MonsterIdleState::Enter(MonsterAIComponent& ai) const
{
    ai.ClearTarget();
    ai.StopMoving();
}

optional<MonsterStateType> MonsterIdleState::Update(MonsterAIComponent& ai) const
{
    if (optional<MonsterStateType> engage = EngageIfTargetFound(ai))
        return engage;

    if (ai.GetStateTime() >= IDLE_TIME)
        return MonsterStateType::Wandering;

    return nullopt;
}

void MonsterWanderingState::Enter(MonsterAIComponent& ai) const
{
    ai.WanderInSpawnRadius();
}

optional<MonsterStateType> MonsterWanderingState::Update(MonsterAIComponent& ai) const
{
    if (optional<MonsterStateType> engage = EngageIfTargetFound(ai))
        return engage;

    // 배회 시간이 끝났거나 목적지에 닿으면 대기한다.
    if (ai.GetStateTime() >= WANDERING_TIME || ai.IsMoving() == false)
        return MonsterStateType::Idle;

    return nullopt;
}

void MonsterChasingState::Enter(MonsterAIComponent& ai) const
{
    ai.ChaseTarget();
}

optional<MonsterStateType> MonsterChasingState::Update(MonsterAIComponent& ai) const
{
    // 추적 한계는 대상이 아니라 스폰 지점에서 잰다. 대상과의 거리로 재면 달아나는 플레이어가 몬스터를 한없이 끌고 간다.
    if (ai.IsBeyondChaseLimit())
        return MonsterStateType::Returning;

    if (ai.IsTargetInAttackRange())
        return MonsterStateType::Attacking;

    // 전환하지 않으면 대상의 지금 위치로 길을 다시 찾는다.
    ai.ChaseTarget();
    return nullopt;
}

void MonsterAttackingState::Enter(MonsterAIComponent& ai) const
{
    ai.HoldForAttack();
}

optional<MonsterStateType> MonsterAttackingState::Update(MonsterAIComponent& ai) const
{
    if (ai.IsBeyondChaseLimit())
        return MonsterStateType::Returning;

    if (ai.IsTargetInAttackRange() == false)
        return MonsterStateType::Chasing;

    if (ai.CanAttack())
        ai.NormalAttack();

    return nullopt;
}

void MonsterAttackingState::Tick(MonsterAIComponent& ai, float deltaTime) const
{
    ai.LookAtTarget();
}

void MonsterReturningState::Enter(MonsterAIComponent& ai) const
{
    // 돌아가기로 한 몬스터의 공격은 들어가지 않는다.
    ai.CancelPendingHit();
    ai.ClearTarget();
    ai.ReturnToSpawnPoint();
}

optional<MonsterStateType> MonsterReturningState::Update(MonsterAIComponent& ai) const
{
    // 스폰 반경에 들었거나 돌아갈 길이 없으면 귀환을 마친다. 대상은 찾지 않는다.
    if (ai.IsMoving() == false)
        return MonsterStateType::Idle;

    return nullopt;
}

void MonsterReturningState::Tick(MonsterAIComponent& ai, float deltaTime) const
{
    ai.RecoverHp(deltaTime);
}
