#include "pch.h"
#include "Monster.h"
#include "Gamedata.h"
#include "TickTimer.h"
#include "Room.h"

Monster::Monster()
{
    _isPlayer = false;

    monsterInfo = objectInfo->mutable_monster_info();
    spawnPos = make_shared<Protocol::PosInfo>();
    stateTickTimer = make_shared<TickTimer>();
}

Monster::~Monster()
{
}

void Monster::PostConstructionSetup()
{
    Creature::PostConstructionSetup();

    weak_ptr<Monster> weakSelf = static_pointer_cast<Monster>(shared_from_this());
    tickGroupFuncs[static_cast<int32>(ETickGroup::TG_PrePhysics)].push_back(
        [weakSelf](float deltaTime)
        {
            if (auto self = weakSelf.lock())
            {
                self->stateTickTimer->Tick(deltaTime);
                self->TickStateMachine(deltaTime);
            }
        }
    );
}

void Monster::Tick(float deltaTime)
{
    Creature::Tick(deltaTime);

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

void Monster::TickStateMachine(float deltaTime)
{
    switch (state)
    {
    case MonsterState::Idle:
        ProcessIdle(deltaTime);
        break;
    case MonsterState::Patrolling:
        ProcessPatrolling(deltaTime);
        break;
    case MonsterState::Chasing:
        ProcessChasing(deltaTime);
        break;
    case MonsterState::Attacking:
        ProcessAttacking(deltaTime);
        break;
    case MonsterState::Death:
        ProcessDeath(deltaTime);
        break;
    default:
        ProcessNone();
        break;
    }
}

void Monster::ProcessNone()
{
    cout << "State None" << '\n';
}

void Monster::ProcessIdle(float deltaTime)
{
    // Check Transition Death
    if (monsterInfo->hp() <= 0)
    {
        SetState(MonsterState::Death);
        return;
    }

    // 타겟이 detection 범위에 들어오면 배틀모드로 전환

    // Check Transition Patrolling
    if (stateTickTimer->GetLastTriggered() > 0)
    {
        SetState(MonsterState::Patrolling);
        return;
    }
    
}

void Monster::ProcessPatrolling(float deltaTime)
{
    Move(deltaTime);

    // Transition Idle
    if (stateTickTimer->GetLastTriggered() > 0 || AlreadyArrive())
    {
        SetState(MonsterState::Idle);
        return;
    }
}

void Monster::ProcessAttacking(float deltaTime)
{
    // 전환 조건 체크

    // Transition Chasing: 공격 사거리를 벗어남

    // Transition Idle : 타겟이 죽음

    // Transition 없음: 공격 or 공격 대기
}

void Monster::ProcessChasing(float deltaTime)
{
    // 전환 조건 체크

    // Transition Attacking: 공격 사거리를 들어옴

    // Transition Idle : 타겟이 죽음 or 일정 시간동안 추격 범위 벗어남

    // Transition 없음: 타겟으로 이동
}

void Monster::ProcessDeath(float deltaTime)
{
}

void Monster::SetState(MonsterState updatedState)
{
    state = updatedState;
    bool isRepeated = false;

    switch (state)
    {
    case MonsterState::Idle:
        {
            cout << "Turn into Idle! Monster id: " << objectInfo->object_id() << '\n';
            posInfo->set_state(Protocol::MoveState::MOVE_STATE_IDLE);

            // Set & Start Timer
            stateTickTimer->SetEndTime(STANDING_TIME);
            stateTickTimer->Start(isRepeated);
            break;
        }
    case MonsterState::Patrolling:
        {
            cout << "Turn into Patrolling!id: " << objectInfo->object_id() << '\n';
            posInfo->set_state(Protocol::MoveState::MOVE_STATE_RUN);

            // Set targetPos
            if (shouldReturn)
            {
                targetPos = { spawnPos->x(), spawnPos->y() };
            }
            else
            {
                RoomRef ownerRoom = room.load().lock();
                if (ownerRoom == nullptr)
                    return;

                float patrolDir = Utils::GetRandom(-180.f, 180.f);
                vector2D moveUnitVec = MathUtil::GetUnitVector(patrolDir);

                float nx = (posInfo->x() + moveUnitVec.x) * 1000.f; // 맵 끝을 포인트로 잡기위해 1000000을 곱한다.
                float ny = (posInfo->y() + moveUnitVec.y) * 1000.f;

                targetPos = ownerRoom->ClampLocation(nx, ny);
            }
            
            shouldReturn = !shouldReturn;

            // Set & Start Timer
            stateTickTimer->SetEndTime(PATROL_MOVING_TIME);
            stateTickTimer->Start(isRepeated);
            break;
        }
    case MonsterState::Chasing:
        break;
    case MonsterState::Attacking:
        break;
    case MonsterState::Death:
        break;
    default:
        break;
    }

}

void Monster::Move(float deltaTime)
{
    vector2D moveVec = vector2D{ targetPos.x - posInfo->x(), targetPos.y - posInfo->y() };
    vector2D moveUnitVec = moveVec.GetNormalize();

    float dx = moveUnitVec.x * min(MONSTER_SPEED * deltaTime, moveVec.GetMagnitude());
    float dy = moveUnitVec.y * min(MONSTER_SPEED * deltaTime, moveVec.GetMagnitude());

    vector2D prevPos = { posInfo->x(), posInfo->y() };
    vector2D curPos;

    curPos.x = prevPos.x + dx;
    curPos.y = prevPos.y + dy;

    posInfo->set_x(curPos.x);
    posInfo->set_y(curPos.y);
    posInfo->set_yaw(MathUtil::vectorToYaw(moveUnitVec));

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

    return MathUtil::distance(monsterPos, targetPos, true) < 1.f;
}
