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

    _spawnPos = MathUtil::PosInfoToVector2D(&owner->GetPosInfo());

    // 스폰 정보에 Idle 이동 상태가 실리도록 룸에 들어가기 전에 맞춰 둔다.
    SwitchState(MonsterState::Idle);
}

void MonsterAIComponent::Tick(float deltaTime)
{
    // 틱마다 타이머 갱신
    {
        _stateTimer += deltaTime;
        _timeSinceLastAttack += deltaTime;
        _timeSinceStateUpdate += deltaTime;
    }

    UpdatePendingHit(deltaTime);

    // 상태 전환 판정은 틱보다 긴 주기로 돈다. 남은 시간을 넘기지 않고 0으로 돌리므로 주기가 틱 간격만큼 늦어질 수 있다.
    if (_timeSinceStateUpdate >= UPDATE_STATE_INTERVAL)
    {
        _timeSinceStateUpdate = 0.f;
        EvaluateStateTransition();
    }

    ExecuteStateBehavior(deltaTime);
}

MonsterRef MonsterAIComponent::GetOwner() const
{
    // 이 컴포넌트를 만드는 엔티티는 Monster뿐이다(ADR-0013).
    return static_pointer_cast<Monster>(_owner.lock());
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
        // (Idle, Wandering) -> (chasing, attacking): detection 안에 플레이어 감지
        if (RoomRef ownerRoom = owner->GetRoom())
        {
            PlayerRef player = nullptr;
            float squareDist = 0.f;
            std::tie(player, squareDist) = ownerRoom->FindClosestPlayer(curPos, _detectionRange);

            if (player != nullptr)
            {
                _target = static_pointer_cast<Entity>(player);

                // xxx -> Attacking 또는 Chasing으로 전환
                if (bool InAttackRange = (squareDist <= (_tryAttackRange * _tryAttackRange)))
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

        // Idle -> Wandering으로 상태 전환
        if (_state == MonsterState::Idle && _stateTimer >= IDLE_TIME)
        {
            SwitchState(MonsterState::Wandering);
            return;
        }

        // Wandering -> Idle로 상태 전환
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
        if (IsTargetLost())
        {
            SwitchState(MonsterState::Idle);
            return;
        }

        if (auto target = _target.lock())
        {
            const Protocol::PosInfo* targetPos = &target->GetPosInfo();

            // 전환 조건(attacking): 공격 범위 안에 들어옴
            if (bool InAttackRange = MathUtil::InRange(curPos, targetPos, _tryAttackRange))
            {
                SwitchState(MonsterState::Attacking);
                return;
            }

            // 전환 조건(idle): 타겟이 추적 범위를 벗어남
            if (bool outOfChasingRange = !MathUtil::InRange(curPos, targetPos, _chasingMaxRange))
            {
                SwitchState(MonsterState::Idle);
                return;
            }

            // 전환이 일어나지 않음(Chasing): 목적지를 갱신한다.
            SetDestination(MathUtil::PosInfoToVector2D(targetPos), MIN_APPROACH_DISTANCE);
        }

        break;
    }
    case MonsterState::Attacking:
    {
        if (IsTargetLost())
        {
            SwitchState(MonsterState::Idle);
            return;
        }

        if (auto target = _target.lock())
        {
            const Protocol::PosInfo* targetPos = &target->GetPosInfo();

            // 전환 조건(Chasing): 공격 범위를 벗어남
            if (bool outOfAttackRange = !MathUtil::InRange(curPos, targetPos, _tryAttackRange))
            {
                SwitchState(MonsterState::Chasing);
                return;
            }

            auto ownerRoom = owner->GetRoom();
            if (ownerRoom == nullptr)
                return;

            // 전환 조건(Idle): 타겟이 현재 Room에서 사라졌거나, 범위를 벗어남
            bool outOfChasingRange = MathUtil::InRange(curPos, targetPos, _chasingMaxRange) == false;
            if (ownerRoom->Contains(target->GetEntityId()) == false || outOfChasingRange)
            {
                _target.reset();
                SwitchState(MonsterState::Idle);
                return;
            }

        }
        else
        {
            // 전환 조건(Idle): 타겟이 유효하지 않음
            _target.reset();
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
        // 서있기 세팅(PosInfo 세팅)
        StopMoving(true);

        // Clear Target
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

        // 목적지로 이동 세팅(PosInfo 세팅)
        StartMovingTo(newDest);

        break;
    }
    case MonsterState::Chasing:
    {
        EntityRef entity = _target.lock();
        if (entity == nullptr)
            return;

        vector2D targetPos = MathUtil::PosInfoToVector2D(&entity->GetPosInfo());

        // 타겟으로 이동 세팅(PosInfo 세팅)
        StartMovingTo(targetPos, MIN_APPROACH_DISTANCE);

        break;
    }
    case MonsterState::Attacking:
    {
        EntityRef entity = _target.lock();
        if (entity == nullptr)
            return;

        // 타겟 공격 세팅(PosInfo 세팅)
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
    // Idle과 Death에는 틱마다 할 행동이 없다.
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

    // Look Target
    const Protocol::PosInfo* targetPos = &target->GetPosInfo();
    LookAt(MathUtil::PosInfoToVector2D(targetPos));

    if (_timeSinceLastAttack >= _attackInterval)
    {
        if (bool InAttackRange = MathUtil::InRange(&owner->GetPosInfo(), targetPos, _tryAttackRange))
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

    // 타겟이랑 충분히 멀리 떨어져 있을때만 이동
    const Protocol::PosInfo* targetPos = &target->GetPosInfo();
    if (bool tooClose = MathUtil::InRange(&owner->GetPosInfo(), targetPos, MIN_APPROACH_DISTANCE))
    {
        SwitchState(MonsterState::Attacking);   // 이 경우, 예외로 상태를 변경한다.
    }
    else
    {
        if (CanMove())
        {
            Move(deltaTime);
        }
    }
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

    // 필요할지도 모르니까 매번 이동 방향 세팅
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

        // 이미 너무 가까움 → 그냥 현 위치를 목적지로 설정.
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
    _moveDest.reset();  // 목적지를 없앤다.

    // 목적지가 없으니 이동도 X
    if (MonsterRef owner = GetOwner())
        owner->SetMoveDirection(vector2D::GetZeroVector());
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

        // TEMP
        Protocol::AttackInfo attackInfo;
        {
            attackInfo.set_type(Protocol::ATTACK_TYPE_NORMAL);
            attackInfo.set_target_id(target->GetEntityId());
            attackInfo.set_combo(0);
            attackInfo.set_damage(_baseAttack);
        }
        _pendingHit = PendingHit{ attackInfo, NORMAL_ATTACK_HIT_DELAY };
    }
}

void MonsterAIComponent::UpdatePendingHit(float deltaTime)
{
    if (_pendingHit.has_value() == false)
        return;

    _pendingHit->remainingTime -= deltaTime;
    if (_pendingHit->remainingTime > 0.f)
        return;

    const Protocol::AttackInfo attackInfo = _pendingHit->attackInfo;
    _pendingHit.reset();

    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    if (auto ownerRoom = owner->GetRoom())
        ownerRoom->HandleHit(owner, attackInfo);
}

bool MonsterAIComponent::IsTargetLost()
{
    // 대상이 사망했거나 룸을 떠났으면 추적을 그만둔다. 사망한 플레이어는 룸에 남으므로 사망을 따로 본다.
    EntityRef target = _target.lock();
    if (target == nullptr)
        return true;

    if (CreatureRef creature = dynamic_pointer_cast<Creature>(target); creature && creature->IsDead())
        return true;

    MonsterRef owner = GetOwner();
    RoomRef ownerRoom = owner ? owner->GetRoom() : nullptr;
    return ownerRoom == nullptr || ownerRoom->Contains(target->GetEntityId()) == false;
}

bool MonsterAIComponent::CanMove()
{
    bool hasLeftAttackDelay = _timeSinceLastAttack <= _attackInterval;
    bool hasDest = _moveDest.has_value();     // 반드시 목적지가 있을때만 이동한다.

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
