#pragma once
#include "Game/Entities/Entity.h"
#include "Game/Entities/CombatStats.h"
#include "Game/Data/CreatureCombat.h"

class ActionComponent;

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
    virtual void OnHit(EntityRef attacker, const AttackInfo& attackInfo) override;
    /** 사망 표시만 남긴다. 룸에서 빼는 일은 룸이 맡는다. */
    virtual void OnDie(EntityRef attacker);
    bool IsDead() { return _isDead; }

protected:
    bool _isDead = false;

    //~ 스탯
public:
    bool HasStat(Protocol::StatType statType);

    /** 없는 스탯을 읽으면 안 된다. 있는지 모르면 HasStat으로 먼저 확인한다. */
    int64 GetStatValue(Protocol::StatType statType) const;

    /** 없는 스탯이면 새로 만든다. */
    void SetStatValue(Protocol::StatType statType, const int64& value);

    const Protocol::StatInfo& GetStatInfo() const { return *_statInfo; }

protected:
    unique_ptr<Protocol::StatInfo> _statInfo;

    //~ 전투
public:
    /** 피해량을 정하는 공격력. 피해 유형에 맞는 값을 고르는 일은 판정이 한다. */
    virtual CombatStats GetAttackStats() const;
    /** 피격과 공격 범위를 재는 캡슐. 크리처 판정 데이터가 없으면 크기가 0이다. */
    const CreatureCombat& GetCapsule() const { return _capsule; }

    ActionComponent& GetAction() { return *_action; }
    const ActionComponent& GetAction() const { return *_action; }

protected:
    CreatureCombat _capsule;

private:
    ActionComponentRef _action;

    //~ 서버가 정하는 이동
protected:
    /** 아래 함수는 위치를 서버가 정하는 크리처의 부품만 부른다. 플레이어의 위치는 클라이언트의 이동을 판정해 쓴다. */
    friend class PathFollowingComponent;

    void SetLocation(const vector3D& pos);
    /** 길이와 무관하게 단위 벡터로 쓴다. 영벡터면 멈춘 것이다. */
    void SetMoveDirection(const vector2D& moveVec);
    void SetYaw(float yaw);
    void SetMoveState(Protocol::MoveState moveState);
};

