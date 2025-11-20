#pragma once

enum class ETickGroup : uint8
{
    TG_PrePhysics = 0,      // 물리 시뮬레이션 실행 전
    TG_DuringPhysics,       // 물리 시뮬레이션 실행
    TG_PostPhysics,         // 물리 시뮬레이션 실행 후
    TG_ObjectTick,          // Object::Tick()
    TG_COUNT
};

using TickGroupFunc = function<void(float)>;

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
        object->PostConstructionSetup();

        return object;
    }

    void TickThisGroup(ETickGroup tickGroup, float deltaTime);

protected:
    virtual void PostConstructionSetup();
    virtual void Tick(float deltaTime);

public:
	bool IsPlayer() { return _isPlayer; }

public:
	Protocol::ObjectInfo* objectInfo;
	Protocol::PosInfo* posInfo;

    friend class Room;
	atomic<weak_ptr<Room>> room;

protected:
	bool _isPlayer = false;
    bool _isTickable = true;

    vector<vector<TickGroupFunc>> tickGroupFuncs;
};

