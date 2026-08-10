#include "pch.h"
#include "Creature.h"
#include "Player.h"
#include "Room.h"

Creature::Creature()
{
    statInfo = new Protocol::StatInfo();
}

Creature::~Creature()
{
    delete statInfo;
}

bool Creature::Init()
{
    if (Object::Init() == false)
        return false;

    // ...
    return true;
}

bool Creature::Start()
{
    if (Object::Start() == false)
        return false;

    return true;
}

void Creature::Tick(float deltaTime)
{
    Object::Tick(deltaTime);


}

void Creature::OnHit(ObjectRef attacker, Protocol::AttackInfo attackInfo)
{
    Object::OnHit(attacker, attackInfo);

    auto ownerRoom = room.load().lock();
    if (ownerRoom == nullptr)
        return;

    // TEMP: Hit 발생시 Hp만 깎도록 설정
    int64 damage = attackInfo.damage();
    int64 hp = GetStatValue(Protocol::STAT_TYPE_HP);
    int64 updatedHp = hp - damage;

    SetStatValue(Protocol::STAT_TYPE_HP, max(0, updatedHp));

    if (updatedHp <= 0)
    {
        OnDie(attacker);
    }
}

void Creature::OnDie(ObjectRef attacker)
{
    isDead = true;
}

bool Creature::HasStat(Protocol::StatType statType)
{
    return statInfo->mutable_info()->contains(statType);
}

int64 Creature::GetStatValue(Protocol::StatType statType)
{
    auto* statMappings = statInfo->mutable_info();
    return statMappings->at((int32)statType);
}

Protocol::Stat Creature::GetStat(Protocol::StatType statType)
{
    int64 value = GetStatValue(statType);
    Protocol::Stat stat;
    {
        stat.set_type(statType);
        stat.set_value(value);
    }

    return stat;
}

void Creature::SetStatValue(Protocol::StatType statType, const int64& value)
{
    auto* statMappings = statInfo->mutable_info();
    (*statMappings)[(int32)statType] = value;
}

