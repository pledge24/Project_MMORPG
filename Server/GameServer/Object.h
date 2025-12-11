#pragma once

class Object : public enable_shared_from_this<Object>
{
public:
	Object();
	virtual ~Object();

public:
    template<typename SubClassType>
    static ObjectRef Create()
    {
        ObjectRef object = make_shared<SubClassType>();

        return object;
    }

    virtual bool Init(Protocol::PosInfo* spawnPos = nullptr);
    virtual bool Start();

protected:
    virtual void Tick(float deltaTime);

public:
    /** 이벤트 함수 */
    virtual void OnHit(ObjectRef attacker, Protocol::AttackInfo attackInfo) {};

    /** Bool 함수 */
	bool IsPlayer() { return _isPlayer; }

    /** Getter 함수 */
    uint64 GetPrevTime() { return prevTime; }
    void GetNormalAttackData() {}

    /** Setter 함수 */
    void SetPrevTime(uint64 time) { prevTime = time; }

public:
	Protocol::ObjectInfo* objectInfo;
	Protocol::PosInfo* posInfo;

    friend class Room;
	atomic<weak_ptr<Room>> room;

protected:
	bool _isPlayer = false;
    bool _isTickable = true;

    uint64 prevTime = 0;
    const uint64 OBJECT_TICK_INTERVAL = 50;
};

