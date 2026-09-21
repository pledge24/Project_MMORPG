#include "pch.h"
#include "Monster.h"
#include "Player.h"
#include "Gamedata.h"
#include "Room.h"
#include "TickIntervalTimer.h"
#include "TickTimer.h"

Monster::Monster()
{
    _isPlayer = false;

    _monsterInfo = _entityInfo->mutable_monster_info();
    _attackTimer = new TickTimer();
}

Monster::~Monster()
{
    delete _attackTimer;
}

bool Monster::Init()
{
    if (Creature::Init() == false)
        return false;

    int32 templateId = _entityInfo->monster_info().template_id();
    if (Gamedata::s_monsterDataTable.contains(templateId) == false)
    {
        cout << "Monster's template id is Invalid" << '\n';
        return false;
    }

    const Json& monsterData = Gamedata::s_monsterDataTable[templateId];
    _monsterData = monsterData;

    // cache monster data
    CacheMonsterData();

    // set Init data
    _monsterInfo->set_template_id(templateId);
    _monsterInfo->set_hp(_maxHp);
    _spawnPos = MathUtil::PosInfoToVector2D(_posInfo);

    // set Idle State
    SwitchState(MonsterState::Idle);

    return true;
}

bool Monster::Start()
{
    if (Creature::Start() == false)
        return false;

    UpdateState();

    return true;
}

void Monster::Tick(float deltaTime)
{
    Creature::Tick(deltaTime);

    // 틱마다 타이머 갱신
    {
        _stateTimer += deltaTime;
        _timeSinceLastAttack += deltaTime;
    }

    // 몬스터 AI 실행
    ExecuteStateBehavior(deltaTime);
}

void Monster::OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo)
{
    Creature::OnHit(attacker, attackInfo);
}

void Monster::OnDie(EntityRef attacker)
{
    Creature::OnDie(attacker);
}

int64 Monster::GetExpReward()
{
    using namespace JsonProperty::Monster;

    int64 minExp = _monsterData[ExpReward][MinExp].is_null() ? 0 : static_cast<int64>(_monsterData[ExpReward][MinExp]);
    int64 maxExp = _monsterData[ExpReward][MaxExp].is_null() ? minExp : static_cast<int64>(_monsterData[ExpReward][MaxExp]);

    return Utils::GetRandom(minExp, maxExp);
}

int64 Monster::GetGoldReward()
{
    using namespace JsonProperty::Monster;

    int64 minGold = _monsterData[GoldReward][MinGold].is_null() ? 0 : static_cast<int64>(_monsterData[GoldReward][MinGold]);
    int64 maxGold = _monsterData[GoldReward][MaxGold].is_null() ? minGold : static_cast<int64>(_monsterData[GoldReward][MaxGold]);

    return Utils::GetRandom(minGold, maxGold);
}

void Monster::UpdateState()
{
    switch (_state)
    {
    case MonsterState::Idle:
    case MonsterState::Wandering:
    {
        // (Idle, Wandering) -> (chasing, attacking): detection 안에 플레이어 감지
        if (RoomRef ownerRoom = _room.load().lock())
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
        if (auto target = _target.lock())
        {
            Protocol::PosInfo* curPos = _posInfo;
            Protocol::PosInfo* targetPos = target->_posInfo;
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
        if (auto target = _target.lock())
        {
            Protocol::PosInfo* curPos = _posInfo;
            Protocol::PosInfo* targetPos = target->_posInfo;

            // 전환 조건(Chasing): 공격 범위를 벗어남
            if (bool outOfAttackRange = !MathUtil::InRange(curPos, targetPos, _tryAttackRange))
            {
                SwitchState(MonsterState::Chasing);
                return;
            }

            auto ownerRoom = _room.load().lock();
            if (ownerRoom == nullptr)
                return;

            // 전환 조건(Idle): 타겟이 현재 Room에서 사라졌거나, 범위를 벗어남
            bool outOfChasingRange = MathUtil::InRange(curPos, targetPos, _chasingMaxRange) == false;
            if (ownerRoom->Contains(target->_entityInfo->entity_id()) == false || outOfChasingRange)
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

    if (auto ownerRoom = _room.load().lock())
    {
        ownerRoom->DoTimer(UPDATE_STATE_INTERVAL_MS, [self = static_pointer_cast<Monster>(shared_from_this()), ownerRoom]()
            {
                int64 entityId = self->_entityInfo->entity_id();

                if(ownerRoom->Contains(entityId))
                    self->UpdateState();
            });

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
        RoomRef ownerRoom = _room.load().lock();
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

        vector2D targetPos = MathUtil::PosInfoToVector2D(entity->_posInfo);

        // 타겟으로 이동 세팅(PosInfo 세팅)
        StartMovingTo(targetPos, MIN_APPROACH_DISTANCE);

        break;
    }
    case MonsterState::Attacking:
    {
        EntityRef entity = _target.lock();
        if (entity == nullptr)
            return;

        Protocol::PosInfo* targetPos = entity->_posInfo;

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
    cout << "Invalid State: State None" << '\n';
}

void Monster::ExecuteStateIdle(float deltaTime)
{
    // Just Validate
    Protocol::Vector* moveDirection = _posInfo->mutable_move_direction();
    if (MathUtil::IsZeroVector(moveDirection) == false)
    {
        cout << "State is Idle. But, moveDirection is not zero vector" << '\n';

        return;
    }

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
    Protocol::PosInfo* targetPos = target->_posInfo;
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
    Protocol::PosInfo* targetPos = target->_posInfo;
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
    if (auto ownerRoom = _room.load().lock())
    {
        int32 combo = 0;
        ownerRoom->HandleNormalAttack(combo, static_pointer_cast<Creature>(shared_from_this()));

        // TEMP
        Protocol::AttackInfo attackInfo;
        {
            attackInfo.set_type(Protocol::ATTACK_TYPE_NORMAL);
            attackInfo.set_target_id(_target.lock()->_entityInfo->entity_id());
            attackInfo.set_combo(0);
            attackInfo.set_damage(_baseAttack);
        }
        ownerRoom->DoTimer(200, &Room::HandleHit, shared_from_this(), attackInfo);
    }
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
    {
        cout << "AlreadyArrive: something wrong" << '\n';
        return true;
    }

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
    if (yaw == 0.f)
    {
        cout << "Warning: Yaw is 0.f" << '\n';
    }

    _posInfo->set_yaw(yaw);
}

void Monster::ClearDestination()
{
    _moveDest.reset();  // 목적지를 없앤다.
    SetMoveDirection(vector2D::GetZeroVector());   // 목적지가 없으니 이동도 X 
}

void Monster::PrintMonsterAllData() const
{
    cout << "=====================" << '\n';
    cout << _monsterData.dump(2) << '\n';

    cout << "templateId: " << _templateId << '\n';
    cout << "maxHp: " << _maxHp << '\n';
    cout << "attackInterval: " << _attackInterval << '\n';
    cout << "baseAttack: " << _baseAttack << '\n';
    cout << "attackRange: " << _tryAttackRange << '\n';
    cout << "detectionRange: " << _detectionRange << '\n';
    cout << "chaseRange: " << _chasingMaxRange << '\n';

    cout << _entityInfo->Utf8DebugString() << '\n';

    cout << "======Monster Data End ====" << '\n';
}

void Monster::CacheMonsterData()
{
    using namespace JsonProperty::Monster;

    _templateId = _monsterData[TemplateId].is_null() ? 0 : static_cast<int32>(_monsterData[TemplateId]);
    _maxHp = _monsterData[MaxHp].is_null() ? 0 : static_cast<int32>(_monsterData[MaxHp]);
    _attackInterval = _monsterData[AttackInterval].is_null() ? 100000.f : static_cast<float>(_monsterData[AttackInterval]);
    _baseAttack = _monsterData[BaseAttack].is_null() ? 0 : static_cast<int32>(_monsterData[BaseAttack]);

    _tryAttackRange = _monsterData[TryAttackRange].is_null() ? 0.f : static_cast<float>(_monsterData[TryAttackRange]);
    _detectionRange = _monsterData[DetectionRange].is_null() ? 0.f : static_cast<float>(_monsterData[DetectionRange]);
    _chasingMaxRange = _monsterData[ChasingMaxRange].is_null() ? 0.f : static_cast<float>(_monsterData[ChasingMaxRange]);
    _monsterSpeed = _monsterData[MonsterSpeed].is_null() ? 0.f : static_cast<float>(_monsterData[MonsterSpeed]);
    _isTargeting = _monsterData[IsTargeting].is_null() ? false : static_cast<bool>(_monsterData[IsTargeting]);
}
