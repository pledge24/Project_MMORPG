#pragma once
#include "Game/Entities/Creature.h"

class TickIntervalTimer;
class TickTimer;
class MonsterAIComponent;

/** 몬스터의 스폰 매개변수. spawnPos는 위치와 함께 배회의 기준점이 된다. */
struct MonsterSpawnParams : public Creature::SpawnParams
{
    int32 templateId = 0;
    Protocol::PosInfo spawnPos;
};

/**
 * 게임 기획 데이터의 몬스터 표로 만드는 AI 크리처. 행동은 MonsterAIComponent가 정한다.
 * Room::SpawnEntity가 만들어 룸에 넣고, 모든 처리가 그 룸 큐 위에서 돈다.
 * 룸의 _entities가 붙잡고, 사망하면 룸이 빼낸다. 빠지면 룸 틱을 받지 않으므로 AI도 멈춘다.
 */
class Monster : public Creature
{
public:
    using SpawnParams = MonsterSpawnParams;

	Monster();
	virtual ~Monster();

protected:
    friend class EntityFactory;
    /** 몬스터 표에 없는 templateId면 false. */
    bool Init(const SpawnParams& params);

    //~ Begin Entity Interface
    virtual void Start() override;
    /** 받은 틱을 AI에 넘긴다. */
    virtual void Tick(float deltaTime) override;
    //~ End Entity Interface

public:
    //~ 이벤트
    /** 스폰 정보에 실리는 HP 사본(monster_info.hp)도 함께 맞춘다. */
    virtual void OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo) override;
    virtual void OnDie(EntityRef attacker) override;

    //~ Getter
    int64 GetTemplateId() { return _templateId; }
    /** 부를 때마다 표의 최솟값과 최댓값 사이에서 새로 뽑는다. */
    int64 GetExpReward();
    /** 부를 때마다 표의 최솟값과 최댓값 사이에서 새로 뽑는다. */
    int64 GetGoldReward();

    //~ 컴포넌트. 읽기만 연다.
    const MonsterAIComponent& GetAI() const { return *_ai; }

private:
    //~ AI가 쓰는 위치 상태. MonsterAIComponent만 부른다.
    friend class MonsterAIComponent;

    /** x와 y만 바꾼다. 높이는 스폰 때의 값을 유지한다. */
    void SetPlanePos(const vector2D& pos);
    /** 길이와 무관하게 단위 벡터로 쓴다. 영벡터면 멈춘 것이다. */
    void SetMoveDirection(const vector2D& moveVec);
    void SetYaw(float yaw);
    void SetMoveState(Protocol::MoveState moveState);

    //~ 기타
    void PrintMonsterAllData() const;
    /** _template에서 스탯 값을 읽어 멤버에 둔다. */
    void CacheMonsterData();

private:
    //~ Monster Template
    /** 몬스터 표의 행 사본. 보상은 여기서 뽑는다. */
    MonsterTemplate _template;

    //~ Monster Stat Data
    /** _entityInfo 안의 monster_info를 가리킨다. 따로 지우지 않는다. */
    Protocol::MonsterInfo* _monsterInfo;
    int32 _templateId;
    int32 _maxHp;

    //~ 컴포넌트
    /** Init에서 만든다. */
    MonsterAIComponentRef _ai;

    TickTimer* _attackTimer = nullptr;          // 사용 안하는 중
};
