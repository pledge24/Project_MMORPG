#pragma once
#include "Entity.h"

class Creature : public Entity
{
public:
	Creature();
	virtual ~Creature();

public:
    virtual bool Init() override;
    virtual bool Start() override;

protected:
    virtual void Tick(float deltaTime) override;

public:
    /** 이벤트 함수 */
    virtual void OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo) override;
    virtual void OnDie(EntityRef attacker);

    bool IsDead() { return _isDead; }
    bool HasStat(Protocol::StatType statType);

    /** Getter 함수 */
    int64 GetStatValue(Protocol::StatType statType);
    Protocol::Stat GetStat(Protocol::StatType statType);

    /** Setter 함수 */
    void SetStatValue(Protocol::StatType statType, const int64& value);

    Protocol::StatInfo* _statInfo;

protected:
    bool _isDead = false;
};

