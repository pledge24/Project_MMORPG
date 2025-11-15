#pragma once
#include "Creature.h"

class Monster : public Creature
{
public:
	Monster();
	virtual ~Monster();

public:
    void Init();

private:
    void CacheMonsterData();

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
    float attackRange;
    float detectionRange;
    float chaseRange;
};

