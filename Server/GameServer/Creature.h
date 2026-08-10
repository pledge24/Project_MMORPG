#pragma once
#include "Object.h"

class Creature : public Object
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
    virtual void OnHit(ObjectRef attacker, Protocol::AttackInfo attackInfo) override;
    virtual void OnDie(ObjectRef attacker);

    bool IsDead() { return isDead; }
    bool HasStat(Protocol::StatType statType);

    /** Getter 함수 */
    int64 GetStatValue(Protocol::StatType statType);
    Protocol::Stat GetStat(Protocol::StatType statType);

    /** Setter 함수 */
    void SetStatValue(Protocol::StatType statType, const int64& value);

    Protocol::StatInfo* statInfo;

protected:
    bool isDead = false;
};

