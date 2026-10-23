#include "Core/pch.h"
#include "Game/AI/MonsterAIComponent.h"
#include "Game/Entities/Monster.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"

MonsterAIComponent::MonsterAIComponent(MonsterRef owner, const MonsterTemplate& monsterTemplate)
    : EntityComponent(owner)
{
    _attackInterval = monsterTemplate.attackInterval;
    _baseAttack = monsterTemplate.baseAttack;
    _tryAttackRange = monsterTemplate.tryAttackRange;
    _detectionRange = monsterTemplate.detectionRange;
    _chasingMaxRange = monsterTemplate.chasingMaxRange;
    _monsterSpeed = monsterTemplate.movementSpeed;

    // 스폰 정보에 Idle 이동 상태가 실리도록 룸에 들어가기 전에 맞춰 둔다.
    SwitchState(MonsterState::Idle);
}

void MonsterAIComponent::Tick(float deltaTime)
{
    // 타이머 갱신.
    {
        _stateTimer += deltaTime;
        _timeSinceLastAttack += deltaTime;
        _timeSinceStateUpdate += deltaTime;
    }

    // 예약한 피격 판정.
    UpdatePendingHit(deltaTime);

    // 대상의 위치는 대상이 이 룸에 있을 때만 읽을 수 있다. 다른 룸으로 떠난 대상의 위치는 그 룸 큐가 쓴다.
    // 그래서 대상 확인은 상태 전환 판정 주기를 기다리지 않고 틱마다 한다.
    if (_target.expired() == false && IsTargetLost())
        SwitchState(MonsterState::Idle);

    // 상태 전환 판정은 틱보다 긴 주기로 돈다. 남은 시간을 넘기지 않고 0으로 돌리므로 주기가 틱 간격만큼 늦어질 수 있다.
    if (_timeSinceStateUpdate >= UPDATE_STATE_INTERVAL)
    {
        _timeSinceStateUpdate = 0.f;
        EvaluateStateTransition();
    }

    // 상태별 행동.
    ExecuteStateBehavior(deltaTime);
}

void MonsterAIComponent::EvaluateStateTransition()
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    const Protocol::PosInfo* curPos = &owner->GetPosInfo();

    switch (_state)
    {
    case MonsterState::Idle:
    case MonsterState::Wandering:
    {
        // 대기와 배회 중에 감지 범위 안의 플레이어를 찾으면 추적이나 공격으로 바꾼다.
        if (RoomRef ownerRoom = owner->GetRoom())
        {
            PlayerRef player = nullptr;
            float squareDist = 0.f;
            std::tie(player, squareDist) = ownerRoom->FindClosestPlayer(curPos, _detectionRange);

            if (player != nullptr)
            {
                _target = static_pointer_cast<Entity>(player);

                // 공격 사거리 안이면 공격, 밖이면 추적으로 바꾼다.
                if (bool inAttackRange = (squareDist <= (_tryAttackRange * _tryAttackRange)))
                {
                    SwitchState(MonsterState::Attacking);
                }
                else
                {
                    SwitchState(MonsterState::Chasing);
                }

                return;
            }
        }

        // 대기 시간이 끝나면 배회한다.
        if (_state == MonsterState::Idle && _stateTimer >= IDLE_TIME)
        {
            SwitchState(MonsterState::Wandering);
            return;
        }

        // 배회 시간이 끝났거나 목적지에 닿으면 대기한다.
        if (_state == MonsterState::Wandering)
        {
            if (_stateTimer >= WANDERING_TIME || AlreadyArrive())
            {
                SwitchState(MonsterState::Idle);
                return;
            }
        }

        break;
    }
    case MonsterState::Chasing:
    {
        // 대상이 룸을 떠났거나 사망하면 대기한다.
        // 이 검사를 통과해야 대상이 살아 있고 이 룸에 있다. 아래는 그것을 믿고 대상을 바로 읽으므로 검사를 뒤로 미루면 안 된다.
        if (IsTargetLost())
        {
            SwitchState(MonsterState::Idle);
            return;
        }

        const Protocol::PosInfo* targetPos = &_target.lock()->GetPosInfo();

        // 공격 범위 안에 들어오면 공격한다.
        if (MathUtil::InRange(curPos, targetPos, _tryAttackRange))
        {
            SwitchState(MonsterState::Attacking);
            return;
        }

        // 대상이 추적 범위를 벗어나면 대기한다.
        if (MathUtil::InRange(curPos, targetPos, _chasingMaxRange) == false)
        {
            SwitchState(MonsterState::Idle);
            return;
        }

        // 전환하지 않으면 추적 목적지를 대상 위치로 갱신한다.
        SetDestination(MathUtil::PosInfoToVector2D(targetPos), MIN_APPROACH_DISTANCE);
        break;
    }
    case MonsterState::Attacking:
    {
        // 대상이 룸을 떠났거나 사망하면 대기한다. 추적과 같이, 이 검사를 통과해야 아래에서 대상을 바로 읽을 수 있다.
        if (IsTargetLost())
        {
            SwitchState(MonsterState::Idle);
            return;
        }

        const Protocol::PosInfo* targetPos = &_target.lock()->GetPosInfo();

        // 공격 범위를 벗어나면 추적한다.
        if (MathUtil::InRange(curPos, targetPos, _tryAttackRange) == false)
        {
            SwitchState(MonsterState::Chasing);
            return;
        }

        // 공격 범위가 추적 범위보다 넓은 표에서만 일어난다. 그때는 추적 범위를 따른다.
        if (MathUtil::InRange(curPos, targetPos, _chasingMaxRange) == false)
        {
            SwitchState(MonsterState::Idle);
            return;
        }

        break;
    }
    default:
        break;
    }
}

void MonsterAIComponent::SwitchState(MonsterState nextState)
{
    _state = nextState;
    _stateTimer = 0.f;

    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    switch (_state)
    {
    case MonsterState::Idle:
    {
        StopMoving(true);

        {
            ClearDestination();
            _target.reset();
        }

        break;
    }
    case MonsterState::Wandering:
    {
        RoomRef ownerRoom = owner->GetRoom();
        if (ownerRoom == nullptr)
            return;

        vector2D newDest = ownerRoom->GetRandomLocation();

        StartMovingTo(newDest);

        break;
    }
    case MonsterState::Chasing:
    {
        EntityRef entity = _target.lock();
        if (entity == nullptr)
            return;

        vector2D targetPos = MathUtil::PosInfoToVector2D(&entity->GetPosInfo());

        StartMovingTo(targetPos, MIN_APPROACH_DISTANCE);

        break;
    }
    case MonsterState::Attacking:
    {
        EntityRef entity = _target.lock();
        if (entity == nullptr)
            return;

        {
            ClearDestination();
            LookAt(MathUtil::PosInfoToVector2D(&entity->GetPosInfo()));
            owner->SetMoveState(Protocol::MoveState::MOVE_STATE_ACTION);
        }

        break;
    }
    default:
        break;
    }
}

void MonsterAIComponent::ExecuteStateBehavior(float deltaTime)
{
    // Idle에는 틱마다 할 행동이 없다.
    switch (_state)
    {
    case MonsterState::Wandering:
        ExecuteStateWandering(deltaTime);
        break;
    case MonsterState::Chasing:
        ExecuteStateChasing(deltaTime);
        break;
    case MonsterState::Attacking:
        ExecuteStateAttacking(deltaTime);
        break;
    default:
        break;
    }
}

void MonsterAIComponent::ExecuteStateWandering(float deltaTime)
{
    if (CanMove())
    {
        Move(deltaTime);
    }
}

void MonsterAIComponent::ExecuteStateAttacking(float deltaTime)
{
    MonsterRef owner = GetOwner();
    auto target = _target.lock();
    if (owner == nullptr || target == nullptr)
        return;

    const Protocol::PosInfo* targetPos = &target->GetPosInfo();
    LookAt(MathUtil::PosInfoToVector2D(targetPos));

    if (_timeSinceLastAttack >= _attackInterval)
    {
        if (bool inAttackRange = MathUtil::InRange(&owner->GetPosInfo(), targetPos, _tryAttackRange))
        {
            _timeSinceLastAttack = 0.f;
            NormalAttack();
        }
    }
}

void MonsterAIComponent::ExecuteStateChasing(float deltaTime)
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    EntityRef target = _target.lock();
    if (target == nullptr)
    {
        StopMoving();
        return;
    }

    // 너무 가까우면 상태 전환 판정 주기를 기다리지 않고 여기서 바로 공격으로 바꾼다.
    const Protocol::PosInfo* targetPos = &target->GetPosInfo();
    if (bool tooClose = MathUtil::InRange(&owner->GetPosInfo(), targetPos, MIN_APPROACH_DISTANCE))
    {
        SwitchState(MonsterState::Attacking);
    }
    else
    {
        if (CanMove())
        {
            Move(deltaTime);
        }
    }
}

bool MonsterAIComponent::IsTargetLost()
{
    // 사망한 플레이어는 리스폰할 때까지 룸에 남으므로, 룸 소속만 보지 않고 사망도 따로 본다.
    EntityRef target = _target.lock();
    if (target == nullptr)
        return true;

    // 룸을 먼저 본다. 떠난 대상의 사망 표시는 그 대상이 들어간 룸 큐가 쓴다.
    MonsterRef owner = GetOwner();
    RoomRef ownerRoom = owner ? owner->GetRoom() : nullptr;
    if (ownerRoom == nullptr || ownerRoom->Contains(target->GetEntityId()) == false)
        return true;

    CreatureRef creature = dynamic_pointer_cast<Creature>(target);
    return creature && creature->IsDead();
}

void MonsterAIComponent::Move(float deltaTime, bool orientRotationToMovement)
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    if (AlreadyArrive())
    {
        StopMoving();
        return;
    }

    owner->SetMoveState(Protocol::MoveState::MOVE_STATE_RUN);

    vector2D curPos = MathUtil::PosInfoToVector2D(&owner->GetPosInfo());
    vector2D targetPos = _moveDest.value();
    vector2D moveVec = targetPos - curPos;
    vector2D moveUnitVec = moveVec.GetNormalize();

    float dx = moveUnitVec.x * min(_monsterSpeed * deltaTime, moveVec.GetMagnitude());
    float dy = moveUnitVec.y * min(_monsterSpeed * deltaTime, moveVec.GetMagnitude());

    curPos.x += dx;
    curPos.y += dy;

    owner->SetPlanePos(curPos);

    owner->SetMoveDirection(moveVec);

    if (orientRotationToMovement)
    {
        LookAt(targetPos);
    }
}

void MonsterAIComponent::LookAt(const vector2D& targetPos)
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    vector2D curPos = MathUtil::PosInfoToVector2D(&owner->GetPosInfo());
    vector2D lookAtVec = targetPos - curPos;

    if (lookAtVec != vector2D::GetZeroVector())
        owner->SetYaw(MathUtil::VectorToYaw(lookAtVec));
}

void MonsterAIComponent::StartMovingTo(const vector2D& dest, float minApproachDistance)
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    vector2D curPos = MathUtil::PosInfoToVector2D(&owner->GetPosInfo());

    SetDestination(dest, minApproachDistance);
    LookAt(dest);
    owner->SetMoveDirection(dest - curPos);
}

void MonsterAIComponent::StopMoving(bool shouldBeIdle)
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    owner->SetMoveDirection(vector2D::GetZeroVector());
    if (owner->GetPosInfo().state() == Protocol::MoveState::MOVE_STATE_RUN || shouldBeIdle)
    {
        owner->SetMoveState(Protocol::MoveState::MOVE_STATE_IDLE);
    }
}

void MonsterAIComponent::SetDestination(const vector2D& destPos, float minApproachDistance)
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    if (minApproachDistance > 0.f)
    {
        vector2D curPos = MathUtil::PosInfoToVector2D(&owner->GetPosInfo());
        vector2D approachVec = destPos - curPos;
        float dist = approachVec.GetMagnitude();

        if (dist <= minApproachDistance)
        {
            _moveDest = curPos;
            return;
        }

        vector2D unitVec = approachVec.GetNormalize();
        vector2D minApproachVec = unitVec * minApproachDistance;
        _moveDest = curPos + (approachVec - minApproachVec);
    }
    else
    {
        _moveDest = destPos;
    }
}

void MonsterAIComponent::ClearDestination()
{
    _moveDest.reset();

    if (MonsterRef owner = GetOwner())
        owner->SetMoveDirection(vector2D::GetZeroVector());
}

bool MonsterAIComponent::CanMove()
{
    bool hasLeftAttackDelay = _timeSinceLastAttack <= _attackInterval;
    bool hasDest = _moveDest.has_value();

    return !hasLeftAttackDelay && hasDest;
}

bool MonsterAIComponent::AlreadyArrive()
{
    if (_moveDest.has_value() == false)
        return true;

    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return true;

    vector2D monsterPos = MathUtil::PosInfoToVector2D(&owner->GetPosInfo());
    return MathUtil::Distance(monsterPos, _moveDest.value(), true) < 1.f;
}

void MonsterAIComponent::NormalAttack()
{
    MonsterRef owner = GetOwner();
    EntityRef target = _target.lock();
    if (owner == nullptr || target == nullptr)
        return;

    if (auto ownerRoom = owner->GetRoom())
    {
        int32 combo = 0;
        ownerRoom->HandleNormalAttack(combo, owner);

        // 일반 공격은 아직 콤보가 없다. 데미지는 몬스터 표의 기본 공격력이다.
        AttackInfo attackInfo;
        {
            attackInfo.targetId = target->GetEntityId();
            attackInfo.damage = _baseAttack;
        }
        _pendingHit = PendingHit{ attackInfo, target, NORMAL_ATTACK_HIT_DELAY };
    }
}

void MonsterAIComponent::UpdatePendingHit(float deltaTime)
{
    if (_pendingHit.has_value() == false)
        return;

    _pendingHit->remainingTime -= deltaTime;
    if (_pendingHit->remainingTime > 0.f)
        return;

    const PendingHit hit = std::move(_pendingHit.value());
    _pendingHit.reset();

    MonsterRef owner = GetOwner();
    EntityRef target = hit.target.lock();
    if (owner == nullptr || target == nullptr)
        return;

    RoomRef ownerRoom = owner->GetRoom();
    if (ownerRoom == nullptr)
        return;

    // 거리는 피격 시점에 다시 본다. 공격을 알린 뒤 사거리를 빠져나간 대상은 공격을 피한 것이다.
    // 대상이 이 룸을 떠났으면 그 위치를 읽지 않는다.
    if (ownerRoom->Contains(target->GetEntityId()) == false)
        return;
    if (MathUtil::InRange(&owner->GetPosInfo(), &target->GetPosInfo(), _tryAttackRange) == false)
        return;

    ownerRoom->HandleHit(owner, hit.attackInfo);
}

MonsterRef MonsterAIComponent::GetOwner() const
{
    // 이 컴포넌트를 만드는 엔티티는 Monster뿐이라 static_pointer_cast를 쓴다. 다른 엔티티에 붙이면 잘못된 형변환으로 메모리를 깨뜨린다.
    return static_pointer_cast<Monster>(_owner.lock());
}
