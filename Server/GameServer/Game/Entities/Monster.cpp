#include "Core/pch.h"
#include "Game/Entities/Monster.h"
#include "Game/AI/MonsterAIComponent.h"
#include "Game/Navigation/PathFollowingComponent.h"

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
    _spawnPoint = params.spawnPoint;
    _spawnPointIndex = params.spawnPointIndex;

    const int32 templateId = params.templateId;
    const DesignData::MonsterRow* monsterRow = Gamedata::FindMonster(templateId);
    if (monsterRow == nullptr)
    {
        GLogger->Warning("몬스터 표에 없는 템플릿 {}으로 몬스터를 만들 수 없다", templateId);
        return false;
    }

    // 기동할 때 몬스터 표의 몬스터마다 판정 데이터가 있는지 대조했다. 없으면 표를 바꿔 끼운 테스트다.
    const CreatureCombat* combat = Gamedata::FindMonsterCombat(templateId);
    if (combat == nullptr)
    {
        GLogger->Warning("크리처 판정 데이터가 없는 템플릿 {}으로 몬스터를 만들 수 없다", templateId);
        return false;
    }

    _template = *monsterRow;

    CacheMonsterData();

    SetStatValue(Protocol::STAT_TYPE_MAX_HP, _maxHp);
    SetStatValue(Protocol::STAT_TYPE_HP, _maxHp);
    _monsterInfo->set_template_id(templateId);

    // AI가 만들어지며 대기 상태에 들어가 경로 추종 부품을 멈추므로 경로 추종 부품을 먼저 만든다.
    const MonsterRef self = static_pointer_cast<Monster>(shared_from_this());
    _pathFollowing = make_shared<PathFollowingComponent>(self, _template.MovementSpeed, combat->capsuleHalfHeight);
    _ai = make_shared<MonsterAIComponent>(self, _template, *combat);

    return true;
}

void Monster::Start()
{
    Creature::Start();

    _ai->Start();
    _pathFollowing->Start();
}

void Monster::Tick(float deltaTime)
{
    Creature::Tick(deltaTime);

    // AI가 목적지를 정한 뒤에 그 길을 따라 움직인다.
    _ai->Tick(deltaTime);
    _pathFollowing->Tick(deltaTime);
}

void Monster::OnDie(EntityRef attacker)
{
    Creature::OnDie(attacker);
}

Protocol::EntityInfo Monster::GetEntityInfo() const
{
    // HP는 스탯 하나에만 둔다. 나중에 들어온 플레이어도 스폰 정보로 지금 HP를 받는다.
    Protocol::EntityInfo entityInfo = Creature::GetEntityInfo();
    entityInfo.mutable_monster_info()->set_hp(GetStatValue(Protocol::STAT_TYPE_HP));
    return entityInfo;
}

int64 Monster::GetExpReward()
{
    return Utils::GetRandom(_template.MinExp, _template.MaxExp);
}

int64 Monster::GetGoldReward()
{
    return Utils::GetRandom(_template.MinGold, _template.MaxGold);
}

void Monster::CacheMonsterData()
{
    _templateId = _template.Id;
    _maxHp = _template.MaxHp;
}

