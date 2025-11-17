#pragma once
#include "Creature.h"

enum class MonsterState : uint8
{
    Idle = 0,
    Chasing,
    Attacking,
    Death,
    StateCount
};

class Monster : public Creature
{
public:
	Monster();
	virtual ~Monster();

protected:
    virtual void Tick(float deltaSecond) override;

public:
    void Init();
    void PrintMonsterAllData() const;

private:
    void CacheMonsterData();

    void ProcessNone();
    void ProcessIdle();
    void ProcessAttacking();
    void ProcessChasing();
    void ProcessDeath();

private:
    Protocol::MonsterInfo* monsterInfo;

    /** Monster Raw Data */
    Json _monsterData;

    /** Monster Stat Data */
    int32 templateId;
    int32 maxHp;
    float attackSpeed;
    int32 baseAttack;

    /** Monster AI Data */
    MonsterState state = MonsterState::Idle;
    float attackRange;
    float detectionRange;
    float chaseRange;
};

