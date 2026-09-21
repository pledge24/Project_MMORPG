#include "pch.h"
#include "Entity.h"
#include "Room.h"

Entity::Entity()
{
	_entityInfo = new Protocol::EntityInfo();
    _posInfo = _entityInfo->mutable_pos_info();
}

Entity::~Entity()
{
	delete _entityInfo;
}

bool Entity::Init()
{

    return true;
}

bool Entity::Start()
{
    // TODO: Validate

    if (_isTickable)
    {
        if (auto ownerRoom = _room.load().lock())
        {
            ownerRoom->DoTimer(ENTITY_TICK_INTERVAL, &Room::TickEntity, shared_from_this());
        }
    }

    return true;
}

void Entity::Tick(float deltaTime)
{
    if (auto ownerRoom = _room.load().lock())
    {
        ownerRoom->DoTimer(ENTITY_TICK_INTERVAL, &Room::TickEntity, shared_from_this());
    }


}

