#pragma once


class Object : public enable_shared_from_this<Object>
{
public:
	Object();
	virtual ~Object();

public:
    virtual void Tick(float deltaSecond);

public:
	bool IsPlayer() { return _isPlayer; }

public:
	Protocol::ObjectInfo* objectInfo;
	Protocol::PosInfo* posInfo;

	atomic<weak_ptr<Room>> room;
    friend class Room;

protected:
	bool _isPlayer = false;
};

