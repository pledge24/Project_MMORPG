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
    spawnPos = make_shared<Protocol::PosInfo>();
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
    spawnPos->CopyFrom(*posInfo);

    // set Idle State
    SetState(MonsterState::Idle);
}

void Monster::PrintMonsterAllData() const
{
    cout << "=====================" << '\n';
    cout << _monsterData.dump(2) << '\n';

    cout << "templateId: " << templateId << '\n';
    cout << "maxHp: " << maxHp << '\n';
    cout << "attackSpeed: " << attackSpeed << '\n';
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
    attackSpeed = _monsterData[AttackSpeed].is_null() ? 100000.f : static_cast<float>(_monsterData[AttackSpeed]);
    baseAttack = _monsterData[BaseAttack].is_null() ? 0 : static_cast<int32>(_monsterData[BaseAttack]);

    tryAttackRange = _monsterData[TryAttackRange].is_null() ? 0.f : static_cast<float>(_monsterData[TryAttackRange]);
    detectionRange = _monsterData[DetectionRange].is_null() ? 0.f : static_cast<float>(_monsterData[DetectionRange]);
    chasingMaxRange = _monsterData[ChasingMaxRange].is_null() ? 0.f : static_cast<float>(_monsterData[ChasingMaxRange]);
}

void Monster::UpdateState()
{
    cout << "Update State!" << '\n';

    switch (state)
    {
    case MonsterState::Idle:
    case MonsterState::Patrolling:
    {
        // (Idle, Patrolling) -> (chasing, attacking): detection 안에 플레이어 감지
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
                    SetState(MonsterState::Attacking);
                }
                else
                {
                    SetState(MonsterState::Chasing);
                }

                return;
            }
        }

        // Idle -> Patrolling으로 상태 전환
        if (state == MonsterState::Idle && _stateTimer >= IDLE_TIME)
        {
            SetState(MonsterState::Patrolling);
            return;
        }
           
        // Patrolling -> Idle로 상태 전환
        if (state == MonsterState::Patrolling)
        {
            if (_stateTimer >= PATROL_MOVING_TIME || AlreadyArrive())
            {
                SetState(MonsterState::Idle);
                return;
            }
        }

        break;
    }
    case MonsterState::Chasing:
    {
        if (auto target = _target.lock())
        {
            Protocol::PosInfo* targetPos = target->posInfo;
            float squareDist = MathUtil::Distance(posInfo, targetPos, true);

            // 전환 조건(attacking): 공격 범위 안에 들어옴
            bool InAttackRange = squareDist <= (tryAttackRange * tryAttackRange);
            if (InAttackRange)
            {
                SetState(MonsterState::Attacking);
                return;
            }

            // 전환 조건(idle): 타겟이 추적 범위를 벗어남
            bool outOfChasingRange = squareDist > (chasingMaxRange * chasingMaxRange);
            if (outOfChasingRange)
            {
                SetState(MonsterState::Idle);
                return;
            }

            // 전환이 일어나지 않음(Chasing): _targetPos 초기화.
            _targetPos = { target->posInfo->x(), target->posInfo->y() };
        }

        break;
    }
    case MonsterState::Attacking:
    {
        if (auto target = _target.lock())
        {
            Protocol::PosInfo* targetPos = target->posInfo;
            float squareDist = MathUtil::Distance(posInfo, targetPos, true);

            // 전환 조건(Chasing): 공격 범위를 벗어남
            bool outOfAttackRange = squareDist > (tryAttackRange * tryAttackRange);
            if (outOfAttackRange)
            {
                SetState(MonsterState::Chasing);
                return;
            }

            auto ownerRoom = room.load().lock();
            if (ownerRoom == nullptr)
                return;

            // 전환 조건(Idle): 타겟이 현재 Room에서 사라졌거나, 범위를 벗어남
            bool outOfChasingRange = squareDist > (chasingMaxRange * chasingMaxRange);
            if (ownerRoom->Contains(target->objectInfo->object_id()) == false || outOfChasingRange)
            {
                _target.reset();
                SetState(MonsterState::Idle);
                return;
            }

        }
        else
        {
            // 전환 조건(Idle): 타겟이 유효하지 않음
            _target.reset();
            SetState(MonsterState::Idle);
            return;
        }

        break;
    }
    default:
        break;
    }
}

void Monster::SetState(MonsterState updatedState)
{
    state = updatedState;
    _stateTimer = 0.f;

    switch (state)
    {
    case MonsterState::Idle:
    {
        Protocol::MoveState prevState = posInfo->state();

        cout << "Turn into Idle! id: " << objectInfo->object_id() << '\n';
        posInfo->set_state(Protocol::MoveState::MOVE_STATE_IDLE);
        _targetPos = nullopt;

        // 움직이다가 멈춘 경우는 바로 Broadcast
        if (prevState != Protocol::MoveState::MOVE_STATE_IDLE)
        {
            if (auto ownerRoom = room.load().lock())
            {
                Protocol::S_MOVE movePkt; 
                movePkt.add_info()->CopyFrom(*posInfo);

                SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(movePkt);
                ownerRoom->Broadcast(sendBuffer);
            }
        }

        break;
    }
    case MonsterState::Patrolling:
    {
        Protocol::MoveState prevState = posInfo->state();

        cout << "Turn into Patrolling! id: " << objectInfo->object_id() << '\n';

        // Set targetPos
        if (_shouldReturn)
        {
            _targetPos = { spawnPos->x(), spawnPos->y() };
        }
        else
        {
            RoomRef ownerRoom = room.load().lock();
            if (ownerRoom == nullptr)
                return;

            float patrolDir = Utils::GetRandom(-180.f, 180.f);
            vector2D moveUnitVec = MathUtil::GetUnitVector(patrolDir);

            float nx = posInfo->x() + moveUnitVec.x * 10000.f; // 맵 끝을 포인트로 잡기위해 10000을 곱한다.
            float ny = posInfo->y() + moveUnitVec.y * 10000.f;

            _targetPos = ownerRoom->ClampLocation(nx, ny);
        }

        _shouldReturn = !_shouldReturn;

        // posInfo 세팅
        {
            vector2D curPos = { posInfo->x(), posInfo->y() };
            float yaw = MathUtil::VectorToYaw(_targetPos.value() - curPos);

            posInfo->set_yaw(yaw);
            posInfo->set_desired_yaw(yaw);
            posInfo->set_state(Protocol::MoveState::MOVE_STATE_RUN);
        }


        // 멈춰있다가 이동하는 경우는 바로 Broadcast
        if (prevState == Protocol::MoveState::MOVE_STATE_IDLE)
        {
            if (auto ownerRoom = room.load().lock())
            {
                Protocol::S_MOVE movePkt;
                movePkt.add_info()->CopyFrom(*posInfo);

                SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(movePkt);
                ownerRoom->Broadcast(sendBuffer);
            }
        }

        break;
    }
    case MonsterState::Chasing: 
    {
        Protocol::MoveState prevState = posInfo->state();

        cout << "Turn into Chasing! id: " << objectInfo->object_id() << '\n';

        if (ObjectRef object = _target.lock())
        {
            Protocol::PosInfo* targetPos = object->posInfo;
            _targetPos = { targetPos->x(), targetPos->y() };

            // posInfo 세팅
            {
                vector2D curPos = { posInfo->x(), posInfo->y() };
                float yaw = MathUtil::VectorToYaw(_targetPos.value() - curPos);

                posInfo->set_yaw(yaw);
                posInfo->set_desired_yaw(yaw);
                posInfo->set_state(Protocol::MoveState::MOVE_STATE_RUN);
            }

            // 움직이다가 멈춘 경우는 바로 Broadcast
            if (prevState != Protocol::MoveState::MOVE_STATE_IDLE)
            {
                if (auto ownerRoom = room.load().lock())
                {
                    Protocol::S_MOVE movePkt;
                    movePkt.add_info()->CopyFrom(*posInfo);

                    SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(movePkt);
                    ownerRoom->Broadcast(sendBuffer);
                }
            }
        }

        break;
    }
    case MonsterState::Attacking:
    {
        cout << "Turn into Attacking! id: " << objectInfo->object_id() << '\n';

        if (ObjectRef object = _target.lock())
        {
            Protocol::PosInfo* targetPos = object->posInfo;
            _targetPos = { targetPos->x(), targetPos->y() };

            // posInfo 세팅
            {
                LookAtTarget();
                posInfo->set_state(Protocol::MoveState::MOVE_STATE_ACTION);
            }
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
    case MonsterState::Patrolling:
        ExecuteStatePatrolling(deltaTime);
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

void Monster::ExecuteStatePatrolling(float deltaTime)
{
    Move(deltaTime);
}

void Monster::ExecuteStateAttacking(float deltaTime)
{
    if (_stateTimer >= attackSpeed)
    {
        _stateTimer = 0.f;

        Protocol::S_NORMAL_ATTACK normalAttackPkt;
        {
            normalAttackPkt.set_object_id(objectInfo->object_id());
            normalAttackPkt.set_combo(0);
        }

        if (auto ownerRoom = room.load().lock())
        {
            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(normalAttackPkt);
            ownerRoom->Broadcast(sendBuffer);
        }
    }
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
    // 반드시 _targetPos가 있을때만 이동한다.
    if (_targetPos.has_value() == false)
        return;

    vector2D curPos = { posInfo->x() , posInfo->y() };
    vector2D targetPos = _targetPos.value();
    vector2D moveVec = targetPos - curPos;
    vector2D moveUnitVec = moveVec.GetNormalize();

    float dx = moveUnitVec.x * min(MONSTER_SPEED * deltaTime, moveVec.GetMagnitude());
    float dy = moveUnitVec.y * min(MONSTER_SPEED * deltaTime, moveVec.GetMagnitude());

    vector2D prevPos = curPos;
    curPos.x = prevPos.x + dx;
    curPos.y = prevPos.y + dy;

    posInfo->set_x(curPos.x);
    posInfo->set_y(curPos.y);
    posInfo->set_yaw(MathUtil::VectorToYaw(moveUnitVec));
    posInfo->set_desired_yaw(MathUtil::VectorToYaw(moveUnitVec));

    //cout << "Monster MoveTo: " << posInfo->x() << " " << posInfo->y() << " " << posInfo->yaw() <<  '\n';
}

bool Monster::AlreadyArrive()
{
    vector2D monsterPos = vector2D{ posInfo->x(), posInfo->y() };
    
    auto targetPos = _targetPos.value();
    return MathUtil::Distance(monsterPos, targetPos, true) < 1.f;
}

void Monster::UpdateTargetPos()
{
    auto target = _target.lock();
    if (target == nullptr)
    {
        wcout << L"UpdateTargetPos() 실패: Target is nullptr" << '\n';
        return;
    }

    Protocol::PosInfo* targetPos = target->posInfo;
    _targetPos = { targetPos->x(), targetPos->y() };
}

void Monster::LookAtTarget()
{
    vector2D curPos = { posInfo->x() , posInfo->y() };
    vector2D targetPos = _targetPos.value();

    vector2D lookAtVec = targetPos - curPos;

    float yaw = MathUtil::VectorToYaw(lookAtVec);
    posInfo->set_yaw(yaw);
    posInfo->set_desired_yaw(yaw);
}
