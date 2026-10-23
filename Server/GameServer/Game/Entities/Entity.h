#pragma once
#include "Game/Combat/CombatTypes.h"

/** 모든 엔티티에 공통인 스폰 매개변수. */
struct EntitySpawnParams
{
};

/**
 * 룸에 존재하는 모든 엔티티의 기반 클래스.
 *
 * 엔티티의 식별 번호, 위치, 소속 룸을 들고, 생성부터 틱까지의 생명주기를 정한다.
 * 생명주기는 언리얼 액터의 스폰 순서를 따른다.
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

    /** 스폰 매개변수로 엔티티를 채운다. EntityFactory가 id를 쓴 뒤에 부른다. 실패하면 false. */
    bool Init(const SpawnParams& params);
    /** 처음 룸에 들어갈 때 한 번 불린다. 언리얼의 BeginPlay에 대응한다. */
    virtual void Start() {}
    /**
     * 소속 룸이 룸 틱마다 룸 큐 위에서 부른다. deltaTime은 초 단위다.
     * 컴포넌트를 가진 엔티티는 받은 틱을 자기 컴포넌트에 넘긴다.
     */
    virtual void Tick(float deltaTime) {}

    //~ 이벤트
public:
    /** 소속 룸 큐 위에서 Combat::ResolveHit가 부른다. */
    virtual void OnHit(EntityRef attacker, const AttackInfo& attackInfo) {};

    //~ Entity 정보 관련
public:
	bool IsPlayer()                                     { return _isPlayer; }

    int64 GetEntityId() const                           { return _entityInfo->entity_id(); }

    void SetPosInfo(const Protocol::PosInfo& posInfo_)  { _posInfo->CopyFrom(posInfo_); }

    /** 다른 클라이언트에게 보낼 스폰 정보. 위치(pos_info)와 종류별 정보가 들어 있다. */
    virtual Protocol::EntityInfo GetEntityInfo() const  { return *_entityInfo; }
    const Protocol::PosInfo& GetPosInfo() const         { return *_posInfo; }

protected:
	unique_ptr<Protocol::EntityInfo> _entityInfo;
    /** _entityInfo 안의 pos_info를 가리킨다. 따로 지우지 않는다. */
	Protocol::PosInfo* _posInfo;

	bool _isPlayer = false;

private:
    /** 테스트가 준비 단계에서 엔티티 번호를 겹치게 정한다. */
    friend struct EntityTestAccess;

    /** 팩토리가 생성 직후에 한 번 부른다. */
    void SetEntityId(int64 entityId)                    { _entityInfo->set_entity_id(entityId); }

    //~ 소속 룸
public:
    /** 룸에 들어가기 전이거나 룸이 사라졌으면 nullptr. 어느 스레드에서 불러도 된다. */
    RoomRef GetRoom() const                             { return _room.load().lock(); }

protected:
    /** 룸에 넣지 않고 소속 룸만 room으로 정한다. 이 엔티티에 온 요청이 그 룸의 큐에서 돈다. Start는 부르지 않는다. */
    void AssignRoom(const RoomRef& room)                { _room.store(room); }

private:
    /** 룸이 AddEntity에서 JoinRoom을, 룸 틱에서 Tick을 부른다. */
    friend class Room;

    /** 소속 룸을 바꾸고, 처음 룸에 들어가는 엔티티면 Start를 부른다. 룸 큐 위에서 부른다. */
    void JoinRoom(const RoomRef& room);

    /** 룸 큐가 쓰고 세션 스레드(패킷 핸들러, 접속 종료)가 읽으므로 atomic이다. */
	atomic<weak_ptr<Room>> _room;
    /** Start를 이미 불렀으면 true. 룸 이동 때 Start가 다시 불리지 않게 막는다. */
    bool _hasBegunPlay = false;
};

