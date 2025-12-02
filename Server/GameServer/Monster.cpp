#include "pch.h"
#include "Monster.h"
#include "Player.h"
#include "Gamedata.h"
#include "Room.h"
#include "TickIntervalTimer.h"

Monster::Monster()
{
    _isPlayer = false;

    monsterInfo = objectInfo->mutable_monster_info();
    stateIntervalTimer = make_shared<TickIntervalTimer>();
}

Monster::~Monster()
{
}

void Monster::PostConstructionSetup()
{
    Creature::PostConstructionSetup();

    // 몬스터 AI 인터벌 타이머 초기화
    weak_ptr<Monster> weakSelf = static_pointer_cast<Monster>(shared_from_this());
    stateIntervalTimer->Init(UPDATE_STATE_INTERVAL, [weakSelf]()
        {
            if (auto self = weakSelf.lock())
            {
                self->UpdateState();
            }
            
        });

    // 틱마다 타이머 갱신
    tickGroupFuncs[static_cast<int32>(ETickGroup::TG_PreObjectTick)].push_back(
        [weakSelf](float deltaTime)
        {
            if (auto self = weakSelf.lock())
            {
                self->_stateTimer += deltaTime;
                self->_timeSinceLastAttack += deltaTime;
                self->stateIntervalTimer->Tick(deltaTime);
            }
        }
    );
}

void Monster::Tick(float deltaTime)
{
    Creature::Tick(deltaTime);

    // 몬스터 AI 실행
    ExecuteStateBehavior(deltaTime);
}

void Monster::PostInit()
{
    int32 templateId = objectInfo->monster_info().template_id();

    if (Gamedata::MonsterDataTable.contains(templateId) == false)
    {
        cout << "Monster's template id is Invalid" << '\n';
        return;
    }

    const Json& monsterData = Gamedata::MonsterDataTable[templateId];
    _monsterData = monsterData;

    // cache monster data
    CacheMonsterData();

    // set Init data
    monsterInfo->set_template_id(templateId);
    monsterInfo->set_hp(maxHp);
    spawnPos = MathUtil::PosInfoToVector2D(posInfo);

    // set Idle State
    ChangeState(MonsterState::Idle);
}

void Monster::PrintMonsterAllData() const
{
    cout << "=====================" << '\n';
    cout << _monsterData.dump(2) << '\n';

    cout << "templateId: " << templateId << '\n';
    cout << "maxHp: " << maxHp << '\n';
    cout << "attackInterval: " << attackInterval << '\n';
    cout << "baseAttack: " << baseAttack << '\n';
    cout << "attackRange: " << tryAttackRange << '\n';
    cout << "detectionRange: " << detectionRange << '\n';
    cout << "chaseRange: " << chasingMaxRange << '\n';

    cout << objectInfo->Utf8DebugString() << '\n';

    cout << "======Monster Data End ====" << '\n';
}

void Monster::OnHit(ObjectRef attacker, Protocol::HitData& hitData)
{
    auto ownerRoom = room.load().lock();
    if (ownerRoom == nullptr)
        return;

    uint64 damage = hitData.damage();
    int32 updated_hp = static_cast<int32>(monsterInfo->hp() - damage);
    monsterInfo->set_hp(max(0, updated_hp));

    if (updated_hp > 0)
    {
        // Broadcast Hit Packet
        {
            Protocol::S_HIT hitPkt;

            hitPkt.mutable_hit_data()->CopyFrom(hitData);
            hitPkt.set_hp(updated_hp);

            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(hitPkt);
            ownerRoom->Broadcast(sendBuffer);
        }
    }
    else
    {
        uint64 objectId = objectInfo->object_id();

        // Make Die Packet
        Protocol::S_DIE DiePkt;
        {
            DiePkt.set_object_id(objectId);
        }

        ownerRoom->OnDie(DiePkt);
        
        // Trigger OnMonsterKill
        if(PlayerRef player = dynamic_pointer_cast<Player>(attacker))
        {
            uint64 expReward = GetExpReward();
            uint64 goldReward = GetGoldReward();

            player->OnMonsterKill(static_pointer_cast<Monster>(shared_from_this()), expReward, goldReward);
        }
    }
}

void Monster::CacheMonsterData()
{
    using namespace JsonProperty::Monster;

    templateId = _monsterData[TemplateId].is_null() ? 0 : static_cast<int32>(_monsterData[TemplateId]);
    maxHp = _monsterData[MaxHp].is_null() ? 0 : static_cast<int32>(_monsterData[MaxHp]);
    attackInterval = _monsterData[AttackInterval].is_null() ? 100000.f : static_cast<float>(_monsterData[AttackInterval]);
    baseAttack = _monsterData[BaseAttack].is_null() ? 0 : static_cast<int32>(_monsterData[BaseAttack]);

    tryAttackRange = _monsterData[TryAttackRange].is_null() ? 0.f : static_cast<float>(_monsterData[TryAttackRange]);
    detectionRange = _monsterData[DetectionRange].is_null() ? 0.f : static_cast<float>(_monsterData[DetectionRange]);
    chasingMaxRange = _monsterData[ChasingMaxRange].is_null() ? 0.f : static_cast<float>(_monsterData[ChasingMaxRange]);
    monsterSpeed = _monsterData[MonsterSpeed].is_null() ? 0.f : static_cast<float>(_monsterData[MonsterSpeed]);
    isTargeting = _monsterData[IsTargeting].is_null() ? false : static_cast<bool>(_monsterData[IsTargeting]);
}

void Monster::UpdateState()
{
    switch (state)
    {
    case MonsterState::Idle:
    case MonsterState::Wandering:
    {
        // (Idle, Wandering) -> (chasing, attacking): detection 안에 플레이어 감지
        if (RoomRef ownerRoom = room.load().lock())
        {
            PlayerRef player = nullptr;
            float squareDist = 0.f;
            std::tie(player, squareDist) = ownerRoom->FindClosestPlayer(posInfo, detectionRange);

            if (player != nullptr)
            {
                _target = static_pointer_cast<Object>(player);

                // xxx -> Attacking 또는 Chasing으로 전환
                if (bool InAttackRange = (squareDist <= (tryAttackRange * tryAttackRange)))
                {
                    ChangeState(MonsterState::Attacking);
                }
                else
                {
                    ChangeState(MonsterState::Chasing);
                }

                return;
            }
        }

        // Idle -> Wandering으로 상태 전환
        if (state == MonsterState::Idle && _stateTimer >= IDLE_TIME)
        {
            ChangeState(MonsterState::Wandering);
            return;
        }
           
        // Wandering -> Idle로 상태 전환
        if (state == MonsterState::Wandering)
        {
            if (_stateTimer >= WANDERING_TIME || AlreadyArrive())
            {
                ChangeState(MonsterState::Idle);
                return;
            }
        }

        break;
    }
    case MonsterState::Chasing:
    {
        if (auto target = _target.lock())
        {
            Protocol::PosInfo* curPos = posInfo;
            Protocol::PosInfo* targetPos = target->posInfo;
            float squareDist = MathUtil::Distance(curPos, targetPos, true);

            // 전환 조건(attacking): 공격 범위 안에 들어옴
            if (bool InAttackRange = MathUtil::InRange(curPos, targetPos, tryAttackRange))
            {
                ChangeState(MonsterState::Attacking);
                return;
            }

            // 전환 조건(idle): 타겟이 추적 범위를 벗어남
            if (bool outOfChasingRange = !MathUtil::InRange(curPos, targetPos, chasingMaxRange))
            {
                ChangeState(MonsterState::Idle);
                return;
            }

            // 전환이 일어나지 않음(Chasing): 목적지를 갱신한다.
            {
                SetDestination(MathUtil::PosInfoToVector2D(targetPos), MIN_APPROACH_DISTANCE);
                //LookAt(ProtoUtil::PosInfoToVector2D(posInfo));
            }
        }

        break;
    }
    case MonsterState::Attacking:
    {
        if (auto target = _target.lock())
        {
            Protocol::PosInfo* curPos = posInfo;
            Protocol::PosInfo* targetPos = target->posInfo;

            // 전환 조건(Chasing): 공격 범위를 벗어남
            if (bool outOfAttackRange = !MathUtil::InRange(curPos, targetPos, tryAttackRange))
            {
                ChangeState(MonsterState::Chasing);
                return;
            }

            auto ownerRoom = room.load().lock();
            if (ownerRoom == nullptr)
                return;

            // 전환 조건(Idle): 타겟이 현재 Room에서 사라졌거나, 범위를 벗어남
            bool outOfChasingRange = MathUtil::InRange(curPos, targetPos, chasingMaxRange) == false;
            if (ownerRoom->Contains(target->objectInfo->object_id()) == false || outOfChasingRange)
            {
                _target.reset();
                ChangeState(MonsterState::Idle);
                return;
            }

        }
        else
        {
            // 전환 조건(Idle): 타겟이 유효하지 않음
            _target.reset();
            ChangeState(MonsterState::Idle);
            return;
        }

        break;
    }
    default:
        break;
    }
}

void Monster::ChangeState(MonsterState changedState)
{
    state = changedState;
    _stateTimer = 0.f;

    switch (state)
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
        RoomRef ownerRoom = room.load().lock();
        if (ownerRoom == nullptr)
            return;

        vector2D newDest = ownerRoom->GetRandomPos();
        
        // 목적지로 이동 세팅(PosInfo 세팅)
        StartMovingTo(newDest);
        
        break;
    }
    case MonsterState::Chasing: 
    {
        ObjectRef object = _target.lock();
        if (object == nullptr)
            return;

        vector2D targetPos = MathUtil::PosInfoToVector2D(object->posInfo);

        // 타겟으로 이동 세팅(PosInfo 세팅)
        StartMovingTo(targetPos, MIN_APPROACH_DISTANCE);

        break;
    }
    case MonsterState::Attacking:
    {
        ObjectRef object = _target.lock();
        if (object == nullptr)
            return;

        Protocol::PosInfo* targetPos = object->posInfo;

        // 타겟 공격 세팅(PosInfo 세팅)
        {
            ClearDestination();
            LookAt(MathUtil::PosInfoToVector2D(targetPos));
            posInfo->set_state(Protocol::MoveState::MOVE_STATE_ACTION);
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
    switch (state)
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
    Protocol::Vector* moveDirection = posInfo->mutable_move_direction();
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
    Protocol::PosInfo* targetPos = target->posInfo;
    LookAt(MathUtil::PosInfoToVector2D(targetPos));
    
    if (_timeSinceLastAttack >= attackInterval)
    {
        if (bool InAttackRange = MathUtil::InRange(posInfo, targetPos, tryAttackRange))
        {
            _timeSinceLastAttack = 0.f;
            Attack();
        }
    }

}

void Monster::ExecuteStateChasing(float deltaTime)
{
    ObjectRef target = _target.lock();
    if (target == nullptr)
    {
        StopMoving("ExecuteStateChasing:: nullptr Target");
        return;
    }

    // 타겟이랑 충분히 멀리 떨어져 있을때만 이동
    Protocol::PosInfo* curPos = posInfo;
    Protocol::PosInfo* targetPos = target->posInfo;
    if (bool tooClose = MathUtil::InRange(curPos, targetPos, MIN_APPROACH_DISTANCE))
    {
        //StopMoving("ExecuteStateChasing:: too Close");
        ChangeState(MonsterState::Attacking);   // 이 경우, 예외로 상태를 변경한다.
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
        ForceBroadcastMovePkt();
        return;
    }

    if (posInfo->state() == Protocol::MoveState::MOVE_STATE_IDLE)
        ForceBroadcastMovePkt();

    posInfo->set_state(Protocol::MoveState::MOVE_STATE_RUN);

    vector2D curPos = { posInfo->x() , posInfo->y() };
    vector2D targetPos = _moveDest.value();
    vector2D moveVec = targetPos - curPos;
    vector2D moveUnitVec = moveVec.GetNormalize();

    float dx = moveUnitVec.x * min(monsterSpeed * deltaTime, moveVec.GetMagnitude());
    float dy = moveUnitVec.y * min(monsterSpeed * deltaTime, moveVec.GetMagnitude());

    curPos.x += dx;
    curPos.y += dy;

    posInfo->set_x(curPos.x);
    posInfo->set_y(curPos.y);

    // 필요할지도 모르니까 매번 이동 방향 세팅
    SetMoveDirection(moveVec);

    if (orientRotationToMovement)
    {
        LookAt(targetPos);
    }
}

void Monster::LookAt(const vector2D& targetPos)
{
    vector2D curPos = { posInfo->x(), posInfo->y() };
    vector2D lookAtVec = targetPos - curPos;

    if(lookAtVec != vector2D::GetZeroVector())
        SetYaw(MathUtil::VectorToYaw(lookAtVec));
}

void Monster::StartMovingTo(const vector2D& dest, float minApproachDistance)
{
    vector2D curPos = MathUtil::PosInfoToVector2D(posInfo);

    SetDestination(dest, minApproachDistance);
    LookAt(dest);
    SetMoveDirection(dest - curPos);
}

void Monster::StopMoving(string context, bool shouldBeIdle)
{
    SetMoveDirection(vector2D::GetZeroVector());
    if (posInfo->state() == Protocol::MoveState::MOVE_STATE_RUN || shouldBeIdle)
    {
        posInfo->set_state(Protocol::MoveState::MOVE_STATE_IDLE);
        ForceBroadcastMovePkt();
        //cout << "StopMoving::set idle: " << context << '\n';
    }
}

void Monster::Attack()
{
    Protocol::S_NORMAL_ATTACK normalAttackPkt;
    {
        normalAttackPkt.set_object_id(objectInfo->object_id());
        normalAttackPkt.set_combo(0);
        normalAttackPkt.set_yaw(posInfo->yaw());
    }

    if (auto ownerRoom = room.load().lock())
    {
        weak_ptr<Monster> weakSelf = static_pointer_cast<Monster>(shared_from_this());
        ownerRoom->DoTimer(200, [weakSelf]()
            {
                if (auto self = weakSelf.lock())
                {
                    self->OnHitCheck();
                }
            } );

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(normalAttackPkt);
        ownerRoom->Broadcast(sendBuffer);
    }

    // float dist = MathUtil::Distance(targetPos, posInfo);
    // printf("Attack! MyPos(%.2f, %.2f) targetPos(%.2f, %.2f), CurYaw: %.2f, Distance: %.2f \n", posInfo->x(), posInfo->y(), targetPos->x(), targetPos->y(), posInfo->yaw(), dist);
}

bool Monster::CanMove()
{
    bool hasLeftAttackDelay = _timeSinceLastAttack <= attackInterval;
    bool hasDest = _moveDest.has_value() == false;     // 반드시 목적지가 있을때만 이동한다.

    return !hasLeftAttackDelay || !hasDest;
}

bool Monster::AlreadyArrive()
{
    vector2D monsterPos = vector2D{ posInfo->x(), posInfo->y() };
    
    auto targetPos = _moveDest.value();
    return MathUtil::Distance(monsterPos, targetPos, true) < 1.f;
}

void Monster::SetDestination(const vector2D& destPos, float minApproachDistance)
{
    if (minApproachDistance > 0.f)
    {
        vector2D curPos = MathUtil::PosInfoToVector2D(posInfo);
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

void Monster::OnHitCheck()
{
    if (isTargeting)
    {
        auto target = _target.lock();
        if (target == nullptr)
            return;

        uint64 targetObjectId = target->objectInfo->object_id();

        auto ownerRoom = room.load().lock();
        if (ownerRoom == nullptr || ownerRoom->Contains(targetObjectId) == false)
            return;

        Protocol::HitData hitData; 
        {
            hitData.set_attacker_id(objectInfo->object_id());
            hitData.set_target_id(targetObjectId);
            hitData.set_damage_type(Protocol::DamageType::DAMAGE_TYPE_PHYSICAL);
            hitData.set_damage(baseAttack);
        }

        target->OnHit(shared_from_this(), hitData);
    }
}

uint64 Monster::GetExpReward()
{
    using namespace JsonProperty::Monster;

    uint64 minExp = _monsterData[ExpReward][MinExp].is_null() ? 0 : static_cast<uint64>(_monsterData[ExpReward][MinExp]);
    uint64 maxExp = _monsterData[ExpReward][MaxExp].is_null() ? minExp : static_cast<uint64>(_monsterData[ExpReward][MaxExp]);
    
    return Utils::GetRandom(minExp, maxExp);
}

uint64 Monster::GetGoldReward()
{
    using namespace JsonProperty::Monster;

    uint64 minGold = _monsterData[GoldReward][MinGold].is_null() ? 0 : static_cast<uint64>(_monsterData[GoldReward][MinGold]);
    uint64 maxGold = _monsterData[GoldReward][MaxGold].is_null() ? minGold : static_cast<uint64>(_monsterData[GoldReward][MaxGold]);

    return Utils::GetRandom(minGold, maxGold);
}

void Monster::SetMoveDirection(const vector2D& moveVec)
{
    vector2D unitVec = moveVec.GetNormalize();
    Protocol::Vector* moveDirection = posInfo->mutable_move_direction();

    moveDirection->Clear();
    unitVec.CopyTo(moveDirection);
}

void Monster::SetYaw(float yaw)
{
    if (yaw == 0.f)
    {
        cout << "Warning: Yaw is 0.f" << '\n';
    }

    posInfo->set_yaw(yaw);
}

void Monster::ClearDestination()
{
    _moveDest.reset();  // 목적지를 없앤다.
    SetMoveDirection(vector2D::GetZeroVector());   // 목적지가 없으니 이동도 X 
}

void Monster::ForceBroadcastMovePkt()
{
    if (auto ownerRoom = room.load().lock())
    {
        Protocol::S_MOVE movePkt;
        movePkt.add_info()->CopyFrom(*posInfo);

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(movePkt);
        ownerRoom->Broadcast(sendBuffer);
    }
}
