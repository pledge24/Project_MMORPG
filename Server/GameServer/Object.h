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
    uint64 GetPrevTime() { return prevTime; }
    void GetNormalAttackData() {}

    void SetPrevTime(uint64 time) { prevTime = time; }

    virtual void OnHit(ObjectRef attacker, Protocol::AttackInfo attackInfo) {};

	bool IsPlayer() { return _isPlayer; }

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

