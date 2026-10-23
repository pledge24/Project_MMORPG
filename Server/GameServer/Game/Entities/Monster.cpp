#include "Core/pch.h"
#include "Game/Entities/Monster.h"
#include "Game/AI/MonsterAIComponent.h"

Monster::Monster()
{
    _isPlayer = false;

    _monsterInfo = _entityInfo->mutable_monster_info();
}

Monster::~Monster()
{
}

bool Monster::Init(const SpawnParams& params)
{
    if (Creature::Init(params) == false)
        return false;

    SetPosInfo(params.spawnPos);

    const int32 templateId = params.templateId;
    const MonsterTemplate* monsterTemplate = Gamedata::FindMonster(templateId);
    if (monsterTemplate == nullptr)
    {
        GLogger->Warning("몬스터 표에 없는 템플릿 {}으로 몬스터를 만들 수 없다", templateId);
        return false;
    }

    _template = *monsterTemplate;

    CacheMonsterData();

    SetStatValue(Protocol::STAT_TYPE_MAX_HP, _maxHp);
    SetStatValue(Protocol::STAT_TYPE_HP, _maxHp);
    _monsterInfo->set_template_id(templateId);

    // AI는 스폰 위치를 배회의 기준점으로 읽으므로 위치를 쓴 뒤에 만든다.
    _ai = make_shared<MonsterAIComponent>(static_pointer_cast<Monster>(shared_from_this()), _template);

    return true;
}

void Monster::Start()
{
    Creature::Start();

    _ai->Start();
}

void Monster::Tick(float deltaTime)
{
    Creature::Tick(deltaTime);

    _ai->Tick(deltaTime);
}

void Monster::OnDie(EntityRef attacker)
{
    Creature::OnDie(attacker);
}

Protocol::EntityInfo Monster::GetEntityInfo() const
{
    // HP는 스탯 하나에만 둔다. 나중에 들어온 플레이어도 스폰 정보로 지금 HP를 받는다.
    Protocol::EntityInfo entityInfo = Creature::GetEntityInfo();
    entityInfo.mutable_monster_info()->set_hp(GetStatInfo().values().at(Protocol::STAT_TYPE_HP));
    return entityInfo;
}

int64 Monster::GetExpReward()
{
    return Utils::GetRandom(_template.minExp, _template.maxExp);
}

int64 Monster::GetGoldReward()
{
    return Utils::GetRandom(_template.minGold, _template.maxGold);
}

void Monster::CacheMonsterData()
{
    _templateId = _template.templateId;
    _maxHp = _template.maxHp;
}

void Monster::SetPlanePos(const vector2D& pos)
{
    _posInfo->mutable_pos()->set_x(pos.x);
    _posInfo->mutable_pos()->set_y(pos.y);
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

void Monster::SetMoveState(Protocol::MoveState moveState)
{
    _posInfo->set_state(moveState);
}
