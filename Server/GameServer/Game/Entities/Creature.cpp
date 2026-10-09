#include "Core/pch.h"
#include "Game/Entities/Creature.h"

Creature::Creature()
{
    _statInfo = make_unique<Protocol::StatInfo>();
}

Creature::~Creature()
{
}

bool Creature::Init(const SpawnParams& params)
{
    if (Entity::Init(params) == false)
        return false;

    return true;
}

void Creature::Start()
{
    Entity::Start();
}

void Creature::Tick(float deltaTime)
{
    Entity::Tick(deltaTime);
}

void Creature::OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo)
{
    Entity::OnHit(attacker, attackInfo);

    // 피격은 HP만 깎는다. 0 아래로 내려가지 않는다.
    int64 damage = attackInfo.damage();
    int64 hp = GetStatValue(Protocol::STAT_TYPE_HP);
    int64 updatedHp = hp - damage;

    SetStatValue(Protocol::STAT_TYPE_HP, max(0, updatedHp));

    if (updatedHp <= 0)
    {
        OnDie(attacker);
    }
}

void Creature::OnDie(EntityRef attacker)
{
    _isDead = true;
}

bool Creature::HasStat(Protocol::StatType statType)
{
    return _statInfo->mutable_info()->contains(statType);
}

int64 Creature::GetStatValue(Protocol::StatType statType)
{
    auto* statMappings = _statInfo->mutable_info();
    return statMappings->at((int32)statType);
}

void Creature::SetStatValue(Protocol::StatType statType, const int64& value)
{
    auto* statMappings = _statInfo->mutable_info();
    (*statMappings)[(int32)statType] = value;
}

