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
    monsterInfo->set_hp(maxHp);
}

void Monster::PrintMonsterAllData()
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
