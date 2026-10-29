#pragma once
#include "Game/Entities/Creature.h"
#include "Game/Data/Placement.h"

class MonsterAIComponent;
class PathFollowingComponent;

/** 몬스터의 스폰 매개변수. */
struct MonsterSpawnParams : public Creature::SpawnParams
{
    int32 templateId = 0;
    Protocol::PosInfo spawnPos;
    /** 배회하는 반경과 추적 한계를 재는 기준. 마커에서 나오지 않은 몬스터(테스트)는 직접 정한다. */
    SpawnPointPlacement spawnPoint;
    /** 룸의 몇 번째 스폰 마커에서 나왔는지. 사망하면 그 마커에서 리젠한다. 마커에서 나오지 않았으면 -1. */
    int32 spawnPointIndex = -1;
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
    virtual void OnDie(EntityRef attacker) override;

    //~ Entity 정보
public:
    /** 몬스터 정보의 HP는 부를 때의 스탯 값이다. */
    virtual Protocol::EntityInfo GetEntityInfo() const override;

    //~ 몬스터 표
public:
    int64 GetTemplateId() { return _templateId; }
    /** 기본 보상의 경험치. 부를 때마다 표의 최솟값과 최댓값 사이에서 새로 뽑는다. */
    int64 GetExpReward();
    /** 기본 보상의 골드. 부를 때마다 표의 최솟값과 최댓값 사이에서 새로 뽑는다. */
    int64 GetGoldReward();

private:
    void CacheMonsterData();

    /** 몬스터 표의 행 사본. */
    DesignData::MonsterRow _template;

    /** _entityInfo 안의 monster_info를 가리킨다. 따로 지우지 않는다. */
    Protocol::MonsterInfo* _monsterInfo;
    int32 _templateId;
    int32 _maxHp;

    //~ AI
public:
    /** 읽기만 연다. */
    const MonsterAIComponent& GetAI() const { return *_ai; }
    const SpawnPointPlacement& GetSpawnPoint() const { return _spawnPoint; }
    /** 마커에서 나오지 않았으면 -1. */
    int32 GetSpawnPointIndex() const { return _spawnPointIndex; }

private:
    /** AI가 이동 창구(Creature)와 경로 추종 부품을 쓴다. */
    friend class MonsterAIComponent;

    PathFollowingComponent& GetPathFollowing() { return *_pathFollowing; }

    MonsterAIComponentRef _ai;
    PathFollowingComponentRef _pathFollowing;
    /** 스폰 매개변수의 사본. */
    SpawnPointPlacement _spawnPoint;
    int32 _spawnPointIndex = -1;
};
