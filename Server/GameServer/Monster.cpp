#include "pch.h"
#include "Monster.h"
#include "Gamedata.h"

Monster::Monster()
{
    _isPlayer = false;

    monsterInfo = objectInfo->mutable_monster_info();
}

Monster::~Monster()
{
}

void Monster::Tick(float deltaSecond)
{
    Creature::Tick(deltaSecond);

    // Do State Behavior
    switch (state)
    {
    case MonsterState::Idle:
        ProcessIdle(deltaSecond);
        break;
    case MonsterState::Chasing:
        ProcessChasing(deltaSecond);
        break;
    case MonsterState::Attacking:
        ProcessAttacking(deltaSecond);
        break;
    case MonsterState::Death:
        ProcessDeath(deltaSecond);
        break;
    default:
        ProcessNone();
        break;
    }


    //cout << "Monster Tick!" << '\n';

}

void Monster::Init()
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
    monsterInfo->set_hp(maxHp-300);
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

void Monster::ProcessNone()
{
    cout << "State None" << '\n';
}

void Monster::ProcessIdle(float deltaSecond)
{
    // 전환 조건 체크

    // Transition Chasing: 감지 범위에 플레이어 들어옴

    // Transition Death : hp가 0이하로 됨..?

    // Transition 없음
    // Patrol: 2초 이동 -> 5초 정지 -> 2초 반대 이동

}

void Monster::ProcessAttacking(float deltaSecond)
{
    // 전환 조건 체크

    // Transition Chasing: 공격 사거리를 벗어남

    // Transition Idle : 타겟이 죽음

    // Transition 없음: 공격 or 공격 대기
}

void Monster::ProcessChasing(float deltaSecond)
{
    // 전환 조건 체크

    // Transition Attacking: 공격 사거리를 들어옴

    // Transition Idle : 타겟이 죽음 or 일정 시간동안 추격 범위 벗어남

    // Transition 없음: 타겟으로 이동
}

void Monster::ProcessDeath(float deltaSecond)
{
}
