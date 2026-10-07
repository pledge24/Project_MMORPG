#pragma once

/**
 * 룸에 놓이는 모든 오브젝트(플레이어, 몬스터)의 기반 클래스.
 * 룸에 들어간 뒤의 상태는 소속 룸의 JobQueue 위에서만 읽고 쓴다.
 * 룸에 있는 동안은 Room의 _entities가 붙잡고, RemoveEntity로 빠지면 다른 참조가 없는 한 사라진다.
 * 게임 코드는 EntityUtils의 팩토리로 만든다. id 발급과 Init 전 준비를 팩토리가 맡는다.
 */
class Entity : public enable_shared_from_this<Entity>
{
public:
	Entity();
	virtual ~Entity();

public:
    /** EntityUtils 팩토리로 채운 값에 대한 초기화 작업을 진행한다. */
    virtual bool Init();
    /**
     * _isTickable이면 소속 룸 큐에 첫 틱을 예약한다.
     * 이때 _room이 비어 있으면 틱이 시작되지 않으므로, 틱을 도는 엔티티는 _room을 정한 뒤에 부른다.
     */
    virtual bool Start();

protected:
    /**
     * 소속 룸 큐 위에서 Room::TickEntity가 부른다. deltaTime은 초 단위다.
     * 다음 틱을 여기서 예약하므로, 재정의하면 부모의 Tick을 불러야 틱이 이어진다.
     * 룸의 _entities에서 빠지면 다음 틱부터 불리지 않는다.
     */
    virtual void Tick(float deltaTime);

public:
    //~ 이벤트
    /** 소속 룸 큐 위에서 Combat::ResolveHit가 부른다. */
    virtual void OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo) {};

	bool IsPlayer() { return _isPlayer; }

    int64 GetEntityId() const { return _entityInfo->entity_id(); }
    /** 마지막 틱 시각(ms, GetTickCount64 기준)이다. */
    uint64 GetPrevTime() { return _prevTime; }
    void GetNormalAttackData() {}

    void SetPrevTime(uint64 time) { _prevTime = time; }
    void SetPosInfo(const Protocol::PosInfo& posInfo_) { _posInfo->CopyFrom(posInfo_); }
    void SetPos(const Protocol::Vector& pos) { _posInfo->mutable_pos()->CopyFrom(pos); }

public:
    /** 엔티티가 소유한다. 소멸자에서 지운다. */
	Protocol::EntityInfo* _entityInfo;
    /** _entityInfo 안의 pos_info를 가리킨다. 따로 지우지 않는다. */
	Protocol::PosInfo* _posInfo;

    friend class Room;
    /** 소속 룸. 룸 큐가 쓰고 세션 스레드도 읽으므로 atomic이다. 룸에 들어가기 전에는 비어 있다. */
	atomic<weak_ptr<Room>> _room;

protected:
	bool _isPlayer = false;
    /** false면 Start가 틱을 예약하지 않는다. 플레이어는 틱을 돌지 않는다. */
    bool _isTickable = true;

    uint64 _prevTime = 0;
    /** 틱 간격(ms) */
    const uint64 ENTITY_TICK_INTERVAL = 50;
};

