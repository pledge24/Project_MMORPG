#include "Core/pch.h"
#include "Game/Entities/Creature.h"
#include "Game/Action/ActionComponent.h"

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

    _action = make_shared<ActionComponent>(static_pointer_cast<Creature>(shared_from_this()));

    return true;
}

void Creature::Start()
{
    Entity::Start();

    _action->Start();
}

void Creature::Tick(float deltaTime)
{
    Entity::Tick(deltaTime);

    // 진행 중인 액션의 타격 판정. 하위 클래스의 행동(몬스터 AI)보다 먼저 돌아, 이번 틱에 시작한 액션은 다음 틱부터 시간을 센다.
    _action->Tick(deltaTime);
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

CombatStats Creature::GetAttackStats() const
{
    // 공격력 스탯이 없는 크리처는 피해를 주지 않는다.
    const auto& values = _statInfo->values();
    auto read = [&values](Protocol::StatType statType)
    {
        auto it = values.find(static_cast<int32>(statType));
        return it == values.end() ? 0 : it->second;
    };

    CombatStats stats;
    {
        stats.physicalAttack = read(Protocol::STAT_TYPE_PHYSICAL_ATTACK);
        stats.magicalAttack = read(Protocol::STAT_TYPE_MAGICAL_ATTACK);
    }
    return stats;
}

vector3D Creature::GetFootPosition() const
{
    const Protocol::Vector& pos = _posInfo->pos();
    return vector3D(pos.x(), pos.y(), pos.z() - _capsule.capsuleHalfHeight);
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

