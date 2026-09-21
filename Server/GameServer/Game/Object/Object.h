#pragma once

class Object : public enable_shared_from_this<Object>
{
public:
	Object();
	virtual ~Object();

public:
    virtual bool Init();
    virtual bool Start();

protected:
    virtual void Tick(float deltaTime);

public:
    /** 이벤트 함수 */
    virtual void OnHit(ObjectRef attacker, Protocol::AttackInfo attackInfo) {};

    /** Bool 함수 */
	bool IsPlayer() { return _isPlayer; }

    /** Getter 함수 */
    uint64 GetPrevTime() { return _prevTime; }
    void GetNormalAttackData() {}

    /** Setter 함수 */
    void SetPrevTime(uint64 time) { _prevTime = time; }
    void SetPosInfo(const Protocol::PosInfo& posInfo_) { _posInfo->CopyFrom(posInfo_); }
    void SetPos(const Protocol::Vector& pos) { _posInfo->mutable_pos()->CopyFrom(pos); }

public:
	Protocol::EntityInfo* _entityInfo;
	Protocol::PosInfo* _posInfo;

    friend class Room;
	atomic<weak_ptr<Room>> _room;

protected:
	bool _isPlayer = false;
    bool _isTickable = true;

    uint64 _prevTime = 0;
    const uint64 OBJECT_TICK_INTERVAL = 50;
};

