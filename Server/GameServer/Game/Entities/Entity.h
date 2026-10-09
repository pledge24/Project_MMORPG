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

    // 엔티티는 shared_ptr로만 다룬다. 복사하면 같은 id의 엔티티가 둘이 된다.
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;

protected:
    friend class EntityFactory;
    
    /** 스폰 매개변수를 멤버에 연결하는 초기화 함수. EntityFactory가 id를 쓴 뒤에 호출한다. */
    bool Init(const SpawnParams& params);
    /** 언리얼의 BeginPlay에 대응하는 함수. 첫 틱 전 작업을 처리한다. */
    virtual void Start() {}
    /**
     * 소속 룸이 룸 틱마다 룸 큐 위에서 부른다. deltaTime은 초 단위다. 엔티티가 다음 틱을 예약하지 않는다(ADR-0011).
     * 컴포넌트를 가진 엔티티는 받은 틱을 자기 컴포넌트에 넘긴다.
     */
    virtual void Tick(float deltaTime) {}

public:
    //~ 이벤트
    /** 소속 룸 큐 위에서 Combat::ResolveHit가 부른다. */
    virtual void OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo) {};

public:
    //~ Entity 정보 관련.
	bool IsPlayer()                                     { return _isPlayer; }

    int64 GetEntityId() const                           { return _entityInfo->entity_id(); }

    void SetPosInfo(const Protocol::PosInfo& posInfo_)  { _posInfo->CopyFrom(posInfo_); }

    /** 다른 클라이언트에게 보낼 스폰 정보. 위치(pos_info)와 종류별 정보가 들어 있다. */
    const Protocol::EntityInfo& GetEntityInfo() const   { return *_entityInfo; }
    const Protocol::PosInfo& GetPosInfo() const         { return *_posInfo; }

    /**
     * 소속 룸. 룸에 들어가기 전이거나 룸이 사라졌으면 nullptr.
     * 룸 큐(Room::AddEntity)가 쓰고, 세션 스레드(패킷 핸들러, 접속 종료)가 읽는다. 그래서 atomic이다.
     */
    RoomRef GetRoom() const                             { return _room.load().lock(); }

protected:
    /** 엔티티가 소유한다. 파생 클래스만 쓴다. 다른 클래스는 GetEntityInfo로 읽는다. */
	unique_ptr<Protocol::EntityInfo> _entityInfo;
    /** _entityInfo 안의 pos_info를 가리킨다. 따로 지우지 않는다. */
	Protocol::PosInfo* _posInfo;

protected:
	bool _isPlayer = false;

private:
    /** 룸이 AddEntity에서 JoinRoom을, 룸 틱에서 Tick을 부른다. */
    friend class Room;
    /** 테스트가 준비 단계에서 엔티티 번호를 겹치게 정한다(GameServerTests/PlayerTestAccess.h). */
    friend struct EntityTestAccess;

    /** 팩토리가 생성 직후에 한 번 부른다. 위치의 엔티티 id도 함께 쓴다. */
    void SetEntityId(int64 entityId)                    { _entityInfo->set_entity_id(entityId); _posInfo->set_entity_id(entityId); }

    /**
     * Room::AddEntity가 룸 큐 위에서 부른다. 소속 룸을 바꾸고, 처음 룸에 들어가는 엔티티면 Start를 부른다.
     * 룸을 먼저 쓴다. Start가 소속 룸을 읽을 수 있어야 하기 때문이다.
     */
    void JoinRoom(const RoomRef& room);

private:
    /** 소속 룸. GetRoom의 주석에 쓰고 읽는 스레드가 있다. */
	atomic<weak_ptr<Room>> _room;
    /** Start를 이미 불렀으면 true. JoinRoom만 읽고 쓴다. 룸 이동 때 Start가 다시 불리지 않게 막는다. */
    bool _hasBegunPlay = false;
};

