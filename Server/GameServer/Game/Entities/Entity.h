#pragma once

/** 모든 엔티티에 공통인 스폰 매개변수. */
struct EntitySpawnParams
{
};

/**
 * Room에 존재하는 모든 엔티티(플레이어, 몬스터)의 기반 클래스.
 * 생명주기는 언리얼의 스폰 순서를 최대한 따른다.
 */
class Entity : public enable_shared_from_this<Entity>
{
public:
    using SpawnParams = EntitySpawnParams;

	Entity();
	virtual ~Entity();

protected:
    friend class EntityFactory;
    
    /** 스폰 매개변수를 멤버에 연결하는 초기화 함수. EntityFactory가 id를 쓴 뒤에 호출한다. */
    bool Init(const SpawnParams& params);
    /** 언리얼의 BeginPlay에 대응하는 함수. 첫 틱 전 작업을 처리한다. */
    virtual void Start();
    virtual void Tick(float deltaTime);

public:
    //~ 이벤트
    /** 소속 룸 큐 위에서 Combat::ResolveHit가 부른다. */
    virtual void OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo) {};

public:
    //~ Entity 정보 관련.
	bool IsPlayer()                                     { return _isPlayer; }

    int64 GetEntityId() const                           { return _entityInfo->entity_id(); }
    /** 마지막 틱 시각(ms, GetTickCount64 기준)이다. */
    uint64 GetPrevTime()                                { return _prevTime; }
    void GetNormalAttackData()                          {}

    void SetPrevTime(uint64 time)                       { _prevTime = time; }
    void SetPosInfo(const Protocol::PosInfo& posInfo_)  { _posInfo->CopyFrom(posInfo_); }
    void SetPos(const Protocol::Vector& pos)            { _posInfo->mutable_pos()->CopyFrom(pos); }

public:
	Protocol::EntityInfo* _entityInfo;
	Protocol::PosInfo* _posInfo;

    friend class Room;
    /** 소속 룸. 룸 큐가 쓰고 세션 스레드도 읽으므로 atomic이다. 룸에 들어가기 전에는 비어 있다. */
	atomic<weak_ptr<Room>> _room;

protected:
	bool _isPlayer = false;
    bool _isTickable = true;

    uint64 _prevTime = 0;
    /** 틱 간격(ms) */
    const uint64 ENTITY_TICK_INTERVAL = 50;

private:
    /** Start를 이미 불렀으면 true. Room::AddEntity만 읽고 쓴다. 룸 이동 때 Start가 다시 불리지 않게 막는다. */
    bool _hasBegunPlay = false;
};

