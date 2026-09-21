#include "pch.h"
#include "Object.h"
#include "Room.h"

Object::Object()
{
	_entityInfo = new Protocol::EntityInfo();
    _posInfo = _entityInfo->mutable_pos_info();
}

Object::~Object()
{
	delete _entityInfo;
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

