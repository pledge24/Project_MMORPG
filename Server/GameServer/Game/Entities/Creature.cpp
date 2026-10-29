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

void Creature::OnHit(EntityRef attacker, const AttackInfo& attackInfo)
{
    Entity::OnHit(attacker, attackInfo);

    // 피격은 HP만 깎는다. 0 아래로 내려가지 않는다.
    int64 damage = attackInfo.damage;
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
    return _statInfo->mutable_values()->contains(statType);
}

int64 Creature::GetStatValue(Protocol::StatType statType) const
{
    return _statInfo->values().at((int32)statType);
}

void Creature::SetStatValue(Protocol::StatType statType, const int64& value)
{
    auto* statMappings = _statInfo->mutable_values();
    (*statMappings)[(int32)statType] = value;
}

void Creature::SetLocation(const vector3D& pos)
{
    Protocol::Vector* location = _posInfo->mutable_pos();
    location->set_x(pos.x);
    location->set_y(pos.y);
    location->set_z(pos.z);
}

void Creature::SetMoveDirection(const vector2D& moveVec)
{
    vector2D unitVec = moveVec.GetNormalize();
    Protocol::Vector* moveDirection = _posInfo->mutable_move_direction();

    moveDirection->Clear();
    unitVec.CopyTo(moveDirection);
}

void Creature::SetYaw(float yaw)
{
    _posInfo->set_yaw(yaw);
}

void Creature::SetMoveState(Protocol::MoveState moveState)
{
    _posInfo->set_state(moveState);
}

