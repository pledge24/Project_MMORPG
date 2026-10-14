#pragma once
#include "Game/Entities/Creature.h"

class MonsterAIComponent;

/** 몬스터의 스폰 매개변수. */
struct MonsterSpawnParams : public Creature::SpawnParams
{
    int32 templateId = 0;
    Protocol::PosInfo spawnPos;
};

/**
 * 기획 데이터의 몬스터 표로 만드는 AI 크리처.
 *
 * 룸 안에서 스스로 움직이고 싸우며, 처치한 플레이어가 받을 보상을 정한다.
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
protected:
    virtual void Start() override;
    virtual void Tick(float deltaTime) override;
    //~ End Entity Interface

    //~ 이벤트
public:
    virtual void OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo) override;
    virtual void OnDie(EntityRef attacker) override;

    //~ 몬스터 표
public:
    int64 GetTemplateId() { return _templateId; }
    /** 부를 때마다 표의 최솟값과 최댓값 사이에서 새로 뽑는다. */
    int64 GetExpReward();
    /** 부를 때마다 표의 최솟값과 최댓값 사이에서 새로 뽑는다. */
    int64 GetGoldReward();

private:
    void CacheMonsterData();

    /** 몬스터 표의 행 사본. */
    MonsterTemplate _template;

    /** _entityInfo 안의 monster_info를 가리킨다. 따로 지우지 않는다. */
    Protocol::MonsterInfo* _monsterInfo;
    int32 _templateId;
    int32 _maxHp;

    //~ AI
public:
    /** 읽기만 연다. */
    const MonsterAIComponent& GetAI() const { return *_ai; }

private:
    /** 아래 위치 함수는 MonsterAIComponent만 부른다. */
    friend class MonsterAIComponent;

    /** x와 y만 바꾼다. 높이는 스폰 때의 값을 유지한다. */
    void SetPlanePos(const vector2D& pos);
    /** 길이와 무관하게 단위 벡터로 쓴다. 영벡터면 멈춘 것이다. */
    void SetMoveDirection(const vector2D& moveVec);
    void SetYaw(float yaw);
    void SetMoveState(Protocol::MoveState moveState);

    MonsterAIComponentRef _ai;
};
