#pragma once
#include "Game/Entities/Entity.h"

/**
 * 스탯과 생사를 갖는 엔티티.
 *
 * 피격되고 사망할 수 있는 것은 모두 이 클래스를 상속한다.
 */
class Creature : public Entity
{
public:
    using SpawnParams = EntitySpawnParams;

	Creature();
	virtual ~Creature();

protected:
    friend class EntityFactory;
    //~ Begin Entity Interface
protected:
    bool Init(const SpawnParams& params);
    virtual void Start() override;
    virtual void Tick(float deltaTime) override;
    //~ End Entity Interface

    //~ 피격과 사망
public:
    /** 공격의 damage만큼 HP를 깎고, HP가 0이 되면 OnDie를 부른다. */
    virtual void OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo) override;
    /** 사망 표시만 남긴다. 룸에서 빼는 일은 룸이 맡는다. */
    virtual void OnDie(EntityRef attacker);
    bool IsDead() { return _isDead; }

protected:
    bool _isDead = false;

    //~ 스탯
public:
    bool HasStat(Protocol::StatType statType);

    /** 없는 스탯을 읽으면 안 된다. 있는지 모르면 HasStat으로 먼저 확인한다. */
    int64 GetStatValue(Protocol::StatType statType);

    /** 없는 스탯이면 새로 만든다. */
    void SetStatValue(Protocol::StatType statType, const int64& value);

    const Protocol::StatInfo& GetStatInfo() const { return *_statInfo; }

protected:
    unique_ptr<Protocol::StatInfo> _statInfo;
};

