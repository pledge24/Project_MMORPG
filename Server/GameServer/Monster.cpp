#include "pch.h"
#include "Monster.h"
#include "Player.h"
#include "Gamedata.h"
#include "TickIntervalTimer.h"
#include "Room.h"

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
                bool InAttackRange = squareDist <= (tryAttackRange * tryAttackRange);
                if (InAttackRange)
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
            bool InAttackRange = MathUtil::InRange(curPos, targetPos, tryAttackRange);
            if (InAttackRange)
            {
                ChangeState(MonsterState::Attacking);
                return;
            }

            // 전환 조건(idle): 타겟이 추적 범위를 벗어남
            bool outOfChasingRange = MathUtil::InRange(curPos, targetPos, chasingMaxRange) == false;
            if (outOfChasingRange)
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
            bool outOfAttackRange = MathUtil::InRange(curPos, targetPos, tryAttackRange) == false;
            if (outOfAttackRange)
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
        //cout << "Turn into Idle! id: " << objectInfo->object_id() << '\n';

        posInfo->set_state(Protocol::MoveState::MOVE_STATE_IDLE);

        // Clear Target
        {
            _moveDest = nullopt;
            _target.reset();
        }

        ForceBroadcastMovePkt();
        
        break;
    }
    case MonsterState::Wandering:
    {
        //cout << "Turn into Wandering! id: " << objectInfo->object_id() << '\n';
        
        Protocol::MoveState prevState = posInfo->state();

        // Set Destination
        {
            RoomRef ownerRoom = room.load().lock();
            if (ownerRoom == nullptr)
                return;

            vector2D randomPos = ownerRoom->GetRandomPos();
            SetDestination(randomPos);
        }

        // posInfo 세팅해서 MovePkt Broadcast
        {
            const vector2D& Dest = GetDestination();
            LookAt(Dest);
            posInfo->set_state(Protocol::MoveState::MOVE_STATE_RUN);
            
            ForceBroadcastMovePkt();
        }
        
        break;
    }
    case MonsterState::Chasing: 
    {
        Protocol::MoveState prevState = posInfo->state();

        //cout << "Turn into Chasing! id: " << objectInfo->object_id() << '\n';

        // posInfo 세팅해서 MovePkt Broadcast
        if (ObjectRef object = _target.lock())
        {
            Protocol::PosInfo* targetPos = object->posInfo;

            SetDestination(MathUtil::PosInfoToVector2D(targetPos), MIN_APPROACH_DISTANCE);
            LookAt(MathUtil::PosInfoToVector2D(targetPos));
            posInfo->set_state(Protocol::MoveState::MOVE_STATE_RUN);
            
            ForceBroadcastMovePkt();
        }

        break;
    }
    case MonsterState::Attacking:
    {
        //cout << "Turn into Attacking! id: " << objectInfo->object_id() << '\n';

        // Attacking으로 전환되자마자 공격하기 위한 세팅
        _stateTimer = attackInterval;

        // posInfo 세팅해서 MovePkt Broadcast
        if (ObjectRef object = _target.lock())
        {
            Protocol::PosInfo* targetPos = object->posInfo;

            SetDestination(MathUtil::PosInfoToVector2D(targetPos));
            LookAt(MathUtil::PosInfoToVector2D(targetPos));
            posInfo->set_state(Protocol::MoveState::MOVE_STATE_ACTION);

            ForceBroadcastMovePkt();
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
    // Nothing to do
}

void Monster::ExecuteStateWandering(float deltaTime)
{
    Move(deltaTime);
}

void Monster::ExecuteStateAttacking(float deltaTime)
{
    auto target = _target.lock();
    if (target == nullptr)
        return;

    // Look Target
    Protocol::PosInfo* targetPos = target->posInfo;
    LookAt(MathUtil::PosInfoToVector2D(targetPos));
    
    if (_stateTimer >= attackInterval)
    {
        _stateTimer = 0.f;

        Protocol::S_NORMAL_ATTACK normalAttackPkt;
        {
            normalAttackPkt.set_object_id(objectInfo->object_id());
            normalAttackPkt.set_combo(0);
            normalAttackPkt.set_yaw(posInfo->yaw());
        }

        if (auto ownerRoom = room.load().lock())
        {
            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(normalAttackPkt);
            ownerRoom->Broadcast(sendBuffer);
        }
    }

    printf("Attack! MyPos(%f, %f) targetPos(%f, %f), CurYaw %f\n", posInfo->x(), posInfo->y(), targetPos->x(), targetPos->y(), posInfo->yaw());

}

void Monster::ExecuteStateChasing(float deltaTime)
{
    Move(deltaTime);
}

void Monster::ExecuteStateDeath(float deltaTime)
{
}

void Monster::Move(float deltaTime)
{
    // 반드시 목적지가 있을때만 이동한다.
    if (_moveDest.has_value() == false)
        return;

    if (AlreadyArrive())
        return;

    vector2D curPos = { posInfo->x() , posInfo->y() };
    vector2D targetPos = _moveDest.value();
    vector2D moveVec = targetPos - curPos;
    vector2D moveUnitVec = moveVec.GetNormalize();

    float dx = moveUnitVec.x * min(monsterSpeed * deltaTime, moveVec.GetMagnitude());
    float dy = moveUnitVec.y * min(monsterSpeed * deltaTime, moveVec.GetMagnitude());

    vector2D prevPos = curPos;
    curPos.x += dx;
    curPos.y += dy;

    posInfo->set_x(curPos.x);
    posInfo->set_y(curPos.y);
    posInfo->set_yaw(MathUtil::VectorToYaw(moveUnitVec));
    posInfo->set_desired_yaw(MathUtil::VectorToYaw(moveUnitVec));

    //cout << "Monster MoveTo: " << posInfo->x() << " " << posInfo->y() << " " << posInfo->yaw() <<  '\n';
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

void Monster::LookAt(const vector2D& targetPos)
{
    vector2D curPos = { posInfo->x(), posInfo->y() };

    vector2D lookAtVec = targetPos - curPos;

    float yaw = MathUtil::VectorToYaw(lookAtVec);
    posInfo->set_yaw(yaw);
    posInfo->set_desired_yaw(yaw);
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
