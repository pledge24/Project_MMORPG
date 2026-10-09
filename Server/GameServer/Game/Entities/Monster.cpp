#include "Core/pch.h"
#include "Game/Entities/Monster.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"
#include "Utils/TickTimer.h"

Monster::Monster()
{
    _isPlayer = false;

    _entityInfo->set_entity_type(Protocol::EntityType::ENTITY_TYPE_MONSTER);
    _monsterInfo = _entityInfo->mutable_monster_info();
    _attackTimer = new TickTimer();
}

Monster::~Monster()
{
    delete _attackTimer;
}

bool Monster::Init(const SpawnParams& params)
{
    if (Creature::Init(params) == false)
        return false;

    // 스폰 위치를 통째로 복사하면 팩토리가 써 둔 위치의 엔티티 id가 지워지므로 다시 쓴다.
    SetPosInfo(params.spawnPos);
    _posInfo->set_entity_id(GetEntityId());

    const int32 templateId = params.templateId;
    const MonsterTemplate* monsterTemplate = Gamedata::FindMonster(templateId);
    if (monsterTemplate == nullptr)
    {
        GLogger->Warning("몬스터 표에 없는 템플릿 {}으로 몬스터를 만들 수 없다", templateId);
        return false;
    }

    _template = *monsterTemplate;

    // cache monster data
    CacheMonsterData();

    // set Init data
    // 피격 처리는 스탯의 HP를 읽는다. monster_info의 hp는 스폰 정보에 실리는 사본이다.
    SetStatValue(Protocol::STAT_TYPE_MAX_HP, _maxHp);
    SetStatValue(Protocol::STAT_TYPE_HP, _maxHp);
    _monsterInfo->set_template_id(templateId);
    _monsterInfo->set_hp(_maxHp);
    _spawnPos = MathUtil::PosInfoToVector2D(_posInfo);

    // set Idle State
    SwitchState(MonsterState::Idle);

    return true;
}

void Monster::Start()
{
    Creature::Start();
}

void Monster::Tick(float deltaTime)
{
    Creature::Tick(deltaTime);

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

    // 몬스터 AI 실행
    ExecuteStateBehavior(deltaTime);
}

void Monster::OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo)
{
    Creature::OnHit(attacker, attackInfo);

    // 나중에 들어온 플레이어가 받는 스폰 정보에 현재 HP가 실리도록 맞춘다.
    _monsterInfo->set_hp(static_cast<int32>(GetStatValue(Protocol::STAT_TYPE_HP)));
}

void Monster::OnDie(EntityRef attacker)
{
    Creature::OnDie(attacker);
}

int64 Monster::GetExpReward()
{
    return Utils::GetRandom(_template.minExp, _template.maxExp);
}

int64 Monster::GetGoldReward()
{
    return Utils::GetRandom(_template.minGold, _template.maxGold);
}

void Monster::EvaluateStateTransition()
{
    switch (_state)
    {
    case MonsterState::Idle:
    case MonsterState::Wandering:
    {
        // (Idle, Wandering) -> (chasing, attacking): detection 안에 플레이어 감지
        if (RoomRef ownerRoom = GetRoom())
        {
            PlayerRef player = nullptr;
            float squareDist = 0.f;
            std::tie(player, squareDist) = ownerRoom->FindClosestPlayer(_posInfo, _detectionRange);

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
            Protocol::PosInfo* curPos = _posInfo;
            const Protocol::PosInfo* targetPos = &target->GetPosInfo();
            float squareDist = MathUtil::Distance(curPos, targetPos, true);

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
            {
                SetDestination(MathUtil::PosInfoToVector2D(targetPos), MIN_APPROACH_DISTANCE);
                //LookAt(ProtoUtil::PosInfoToVector2D(_posInfo));
            }
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
            Protocol::PosInfo* curPos = _posInfo;
            const Protocol::PosInfo* targetPos = &target->GetPosInfo();

            // 전환 조건(Chasing): 공격 범위를 벗어남
            if (bool outOfAttackRange = !MathUtil::InRange(curPos, targetPos, _tryAttackRange))
            {
                SwitchState(MonsterState::Chasing);
                return;
            }

            auto ownerRoom = GetRoom();
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

void Monster::SwitchState(MonsterState nextState)
{
    _state = nextState;
    _stateTimer = 0.f;

    switch (_state)
    {
    case MonsterState::Idle:
    {
        // 서있기 세팅(PosInfo 세팅)
        StopMoving("MonsterState::Idle", true);

        // Clear Target
        {
            ClearDestination();
            _target.reset();
        }

        break;
    }
    case MonsterState::Wandering:
    {
        RoomRef ownerRoom = GetRoom();
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

        const Protocol::PosInfo* targetPos = &entity->GetPosInfo();

        // 타겟 공격 세팅(PosInfo 세팅)
        {
            ClearDestination();
            LookAt(MathUtil::PosInfoToVector2D(targetPos));
            _posInfo->set_state(Protocol::MoveState::MOVE_STATE_ACTION);
        }

        break;
    }
    case MonsterState::Death:
        break;
    default:
        break;
    }

}

void Monster::ExecuteStateBehavior(float deltaTime)
{
    switch (_state)
    {
    case MonsterState::Idle:
        ExecuteStateIdle(deltaTime);
        break;
    case MonsterState::Wandering:
        ExecuteStateWandering(deltaTime);
        break;
    case MonsterState::Chasing:
        ExecuteStateChasing(deltaTime);
        break;
    case MonsterState::Attacking:
        ExecuteStateAttacking(deltaTime);
        break;
    case MonsterState::Death:
        ExecuteStateDeath(deltaTime);
        break;
    default:
        ExecuteStateNone();
        break;
    }
}

void Monster::ExecuteStateNone()
{
}

void Monster::ExecuteStateIdle(float deltaTime)
{
}

void Monster::ExecuteStateWandering(float deltaTime)
{
    if (CanMove())
    {
        Move(deltaTime);
    }
}

void Monster::ExecuteStateAttacking(float deltaTime)
{
    auto target = _target.lock();
    if (target == nullptr)
        return;

    // Look Target
    const Protocol::PosInfo* targetPos = &target->GetPosInfo();
    LookAt(MathUtil::PosInfoToVector2D(targetPos));
    
    if (_timeSinceLastAttack >= _attackInterval)
    {
        if (bool InAttackRange = MathUtil::InRange(_posInfo, targetPos, _tryAttackRange))
        {
            _timeSinceLastAttack = 0.f;
            NormalAttack();  
        }
    }

}

void Monster::ExecuteStateChasing(float deltaTime)
{
    EntityRef target = _target.lock();
    if (target == nullptr)
    {
        StopMoving("ExecuteStateChasing:: nullptr Target");
        return;
    }

    // 타겟이랑 충분히 멀리 떨어져 있을때만 이동
    Protocol::PosInfo* curPos = _posInfo;
    const Protocol::PosInfo* targetPos = &target->GetPosInfo();
    if (bool tooClose = MathUtil::InRange(curPos, targetPos, MIN_APPROACH_DISTANCE))
    {
        //StopMoving("ExecuteStateChasing:: too Close");
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

void Monster::ExecuteStateDeath(float deltaTime)
{

}

void Monster::Move(float deltaTime, bool orientRotationToMovement)
{
    if (AlreadyArrive())
    {
        StopMoving("Move:: AlreadyArrive");
        //ForceBroadcastMovePkt();
        return;
    }

    if (_posInfo->state() == Protocol::MoveState::MOVE_STATE_IDLE)
    {
        //ForceBroadcastMovePkt();
    }

    _posInfo->set_state(Protocol::MoveState::MOVE_STATE_RUN);

    vector2D curPos = { _posInfo->pos().x() , _posInfo->pos().y() };
    vector2D targetPos = _moveDest.value();
    vector2D moveVec = targetPos - curPos;
    vector2D moveUnitVec = moveVec.GetNormalize();

    float dx = moveUnitVec.x * min(_monsterSpeed * deltaTime, moveVec.GetMagnitude());
    float dy = moveUnitVec.y * min(_monsterSpeed * deltaTime, moveVec.GetMagnitude());

    curPos.x += dx;
    curPos.y += dy;

    _posInfo->mutable_pos()->set_x(curPos.x);
    _posInfo->mutable_pos()->set_y(curPos.y);

    // 필요할지도 모르니까 매번 이동 방향 세팅
    SetMoveDirection(moveVec);

    if (orientRotationToMovement)
    {
        LookAt(targetPos);
    }
}

void Monster::LookAt(const vector2D& targetPos)
{
    vector2D curPos = { _posInfo->pos().x(), _posInfo->pos().y() };
    vector2D lookAtVec = targetPos - curPos;

    if(lookAtVec != vector2D::GetZeroVector())
        SetYaw(MathUtil::VectorToYaw(lookAtVec));
}

void Monster::StartMovingTo(const vector2D& dest, float minApproachDistance)
{
    vector2D curPos = MathUtil::PosInfoToVector2D(_posInfo);

    SetDestination(dest, minApproachDistance);
    LookAt(dest);
    SetMoveDirection(dest - curPos);
}

void Monster::StopMoving(string context, bool shouldBeIdle)
{
    SetMoveDirection(vector2D::GetZeroVector());
    if (_posInfo->state() == Protocol::MoveState::MOVE_STATE_RUN || shouldBeIdle)
    {
        _posInfo->set_state(Protocol::MoveState::MOVE_STATE_IDLE);
        //ForceBroadcastMovePkt();
        //cout << "StopMoving::set idle: " << context << '\n';
    }
}

void Monster::NormalAttack()
{
    EntityRef target = _target.lock();
    if (target == nullptr)
        return;

    if (auto ownerRoom = GetRoom())
    {
        int32 combo = 0;
        ownerRoom->HandleNormalAttack(combo, static_pointer_cast<Creature>(shared_from_this()));

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

void Monster::UpdatePendingHit(float deltaTime)
{
    if (_pendingHit.has_value() == false)
        return;

    _pendingHit->remainingTime -= deltaTime;
    if (_pendingHit->remainingTime > 0.f)
        return;

    const Protocol::AttackInfo attackInfo = _pendingHit->attackInfo;
    _pendingHit.reset();

    if (auto ownerRoom = GetRoom())
        ownerRoom->HandleHit(shared_from_this(), attackInfo);
}

bool Monster::IsTargetLost()
{
    // 대상이 사망했거나 룸을 떠났으면 추적을 그만둔다. 사망한 플레이어는 룸에 남으므로 사망을 따로 본다.
    EntityRef target = _target.lock();
    if (target == nullptr)
        return true;

    if (CreatureRef creature = dynamic_pointer_cast<Creature>(target); creature && creature->IsDead())
        return true;

    RoomRef ownerRoom = GetRoom();
    return ownerRoom == nullptr || ownerRoom->Contains(target->GetEntityId()) == false;
}

bool Monster::CanMove()
{
    bool hasLeftAttackDelay = _timeSinceLastAttack <= _attackInterval;
    bool hasDest = _moveDest.has_value();     // 반드시 목적지가 있을때만 이동한다.

    return !hasLeftAttackDelay && hasDest;
}

bool Monster::AlreadyArrive()
{
    vector2D monsterPos = vector2D{ _posInfo->pos().x(), _posInfo->pos().y() };
    
    if (_moveDest.has_value() == false)
        return true;

    auto targetPos = _moveDest.value();
    return MathUtil::Distance(monsterPos, targetPos, true) < 1.f;
}

bool Monster::IsTargetingAttack(Protocol::AttackType type)
{
    switch (type)
    {
    case Protocol::ATTACK_TYPE_NORMAL:
        return _isTargeting;
    case Protocol::ATTACK_TYPE_SKILL:
    case Protocol::ATTACK_TYPE_EOT:
    default:
        break;
    }

    return false;
}

void Monster::SetDestination(const vector2D& destPos, float minApproachDistance)
{
    if (minApproachDistance > 0.f)
    {
        vector2D curPos = MathUtil::PosInfoToVector2D(_posInfo);
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

void Monster::SetMoveDirection(const vector2D& moveVec)
{
    vector2D unitVec = moveVec.GetNormalize();
    Protocol::Vector* moveDirection = _posInfo->mutable_move_direction();

    moveDirection->Clear();
    unitVec.CopyTo(moveDirection);
}

void Monster::SetYaw(float yaw)
{
    _posInfo->set_yaw(yaw);
}

void Monster::ClearDestination()
{
    _moveDest.reset();  // 목적지를 없앤다.
    SetMoveDirection(vector2D::GetZeroVector());   // 목적지가 없으니 이동도 X 
}

void Monster::PrintMonsterAllData() const
{
    GLogger->Debug("몬스터 templateId: {} · maxHp: {} · attackInterval: {} · baseAttack: {} · attackRange: {} · detectionRange: {} · chaseRange: {}\n{}",
        _templateId, _maxHp, _attackInterval, _baseAttack, _tryAttackRange, _detectionRange, _chasingMaxRange,
        _entityInfo->Utf8DebugString());
}

void Monster::CacheMonsterData()
{
    _templateId = _template.templateId;
    _maxHp = _template.maxHp;
    _attackInterval = _template.attackInterval;
    _baseAttack = _template.baseAttack;

    _tryAttackRange = _template.tryAttackRange;
    _detectionRange = _template.detectionRange;
    _chasingMaxRange = _template.chasingMaxRange;
    _monsterSpeed = _template.movementSpeed;
    _isTargeting = _template.isTargeting;
}
