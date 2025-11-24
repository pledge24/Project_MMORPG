#include "pch.h"
#include "Monster.h"
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
    cout << "attackRange: " << attackRange << '\n';
    cout << "detectionRange: " << detectionRange << '\n';
    cout << "chaseRange: " << chaseRange << '\n';

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

    attackRange = _monsterData[AttackRange].is_null() ? 0.f : static_cast<float>(_monsterData[AttackRange]);
    detectionRange = _monsterData[DetectionRange].is_null() ? 0.f : static_cast<float>(_monsterData[DetectionRange]);
    chaseRange = _monsterData[ChaseRange].is_null() ? 0.f : static_cast<float>(_monsterData[ChaseRange]);
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
                _stateTimer = 0.f;
                _target = static_pointer_cast<Object>(player);

                // 공격 범위 내로 들어온 경우.
                if (squareDist <= attackRange * attackRange)
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
            _stateTimer = 0.f;
            SetState(MonsterState::Patrolling);
            return;
        }
           
        // Patrolling -> Idle로 상태 전환
        if (state == MonsterState::Patrolling)
        {
            if (_stateTimer >= PATROL_MOVING_TIME || AlreadyArrive())
            {
                _stateTimer = 0.f;
                SetState(MonsterState::Idle);
                return;
            }
        }

        break;
    }
    case MonsterState::Chasing:
    {
        // 전환 조건(attacking): 공격 범위 안에 들어옴
        if (auto target = _target.lock())
        {
            Protocol::PosInfo* targetPos = target->posInfo;
            float squareDist = MathUtil::Distance(posInfo, targetPos);

            if (squareDist <= attackRange * attackRange)
            {
                _stateTimer = 0.f;
                SetState(MonsterState::Attacking);
                return;
            }
        }

        // 전환 조건(idle): 타겟이 몇 초 이상 범위를 벗어남
        if (_stateTimer >= QUIT_CHASING_TIME)
        {
            _stateTimer = 0.f;
            SetState(MonsterState::Idle);
            return;
        }

        break;
    }
    case MonsterState::Attacking:
    {
        if (auto target = _target.lock())
        {
            Protocol::PosInfo* targetPos = target->posInfo;
            float squareDist = MathUtil::Distance(posInfo, targetPos);

            // 전환 조건(Chasing): 공격 범위를 벗어남
            if (squareDist > attackRange * attackRange)
            {
                _stateTimer = 0.f;
                SetState(MonsterState::Chasing);
                return;
            }

            // 전환 조건(Idle): 타겟이 현재 Room에서 사라진 경우
            if (auto ownerRoom = room.load().lock())
            {
                if (ownerRoom->Contains(target->objectInfo->object_id()) == false)
                {
                    _target.reset();
                    _stateTimer = 0.f;
                    SetState(MonsterState::Idle);
                    return;
                }
            }

        }
        else
        {
            // 전환 조건(Idle): 타겟이 유효하지 않음
            _target.reset();
            _stateTimer = 0.f;
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

    switch (state)
    {
    case MonsterState::Idle:
    {
        cout << "Turn into Idle! Monster id: " << objectInfo->object_id() << '\n';
        posInfo->set_state(Protocol::MoveState::MOVE_STATE_IDLE);
        break;
    }
    case MonsterState::Patrolling:
    {
        cout << "Turn into Patrolling!id: " << objectInfo->object_id() << '\n';
        posInfo->set_state(Protocol::MoveState::MOVE_STATE_RUN);

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
        break;
    }
    case MonsterState::Chasing: 
    {
        if (ObjectRef object = _target.lock())
        {
            Protocol::PosInfo* targetPos = object->posInfo;
            _targetPos = { targetPos->x(), targetPos->y() };
        }

        break;
    }
    case MonsterState::Attacking:
    {
        if (ObjectRef object = _target.lock())
        {
            Protocol::PosInfo* targetPos = object->posInfo;
            _targetPos = { targetPos->x(), targetPos->y() };
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
    // Do Nothing
}

void Monster::ExecuteStatePatrolling(float deltaTime)
{
    Move(deltaTime);
}

void Monster::ExecuteStateAttacking(float deltaTime)
{
}

void Monster::ExecuteStateChasing(float deltaTime)
{
}

void Monster::ExecuteStateDeath(float deltaTime)
{
}

void Monster::Move(float deltaTime)
{
    vector2D moveVec = vector2D{ _targetPos.x - posInfo->x(), _targetPos.y - posInfo->y() };
    vector2D moveUnitVec = moveVec.GetNormalize();

    float dx = moveUnitVec.x * min(MONSTER_SPEED * deltaTime, moveVec.GetMagnitude());
    float dy = moveUnitVec.y * min(MONSTER_SPEED * deltaTime, moveVec.GetMagnitude());

    vector2D prevPos = { posInfo->x(), posInfo->y() };
    vector2D curPos;

    curPos.x = prevPos.x + dx;
    curPos.y = prevPos.y + dy;

    posInfo->set_x(curPos.x);
    posInfo->set_y(curPos.y);
    posInfo->set_yaw(MathUtil::VectorToYaw(moveUnitVec));

    uint64 objectId = objectInfo->object_id();
    if (RoomRef ownerRoom = room.load().lock())
    {
        ownerRoom->UpdateCellMatrixOnMove(objectId, prevPos, curPos);
    }

    cout << "Monster MoveTo: " << curPos.x << " " << curPos.y << '\n';
}

bool Monster::AlreadyArrive()
{
    vector2D monsterPos = vector2D{ posInfo->x(), posInfo->y() };

    return MathUtil::Distance(monsterPos, _targetPos, true) < 1.f;
}
