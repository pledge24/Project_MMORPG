#include "pch.h"
#include "Object.h"
#include "Room.h"

Object::Object()
{
	_objectInfo = new Protocol::ObjectInfo();
    _posInfo = _objectInfo->mutable_pos_info();
}

Object::~Object()
{
	delete _objectInfo;
}

bool Object::Init()
{

    return true;
}

bool Object::Start()
{
    // TODO: Validate

    if (_isTickable)
    {
        if (auto ownerRoom = _room.load().lock())
        {
            ownerRoom->DoTimer(OBJECT_TICK_INTERVAL, &Room::TickObject, shared_from_this());
        }
    }

    return true;
}

void Object::Tick(float deltaTime)
{
    if (auto ownerRoom = _room.load().lock())
    {
        ownerRoom->DoTimer(OBJECT_TICK_INTERVAL, &Room::TickObject, shared_from_this());
    }


}

