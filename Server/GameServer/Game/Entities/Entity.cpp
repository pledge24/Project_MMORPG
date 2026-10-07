#include "Core/pch.h"
#include "Game/Entities/Entity.h"
#include "Game/Room/Room.h"

Entity::Entity()
{
	_entityInfo = new Protocol::EntityInfo();
    _posInfo = _entityInfo->mutable_pos_info();
}

Entity::~Entity()
{
	delete _entityInfo;
}

bool Entity::Init(const SpawnParams&)
{
    return true;
}

void Entity::Start()
{
    // 첫 틱의 deltaTime은 Start부터 잰다.
    _prevTime = GetTickCount64();

    if (_isTickable)
    {
        if (auto ownerRoom = _room.load().lock())
        {
            ownerRoom->DoTimer(ENTITY_TICK_INTERVAL, &Room::TickEntity, shared_from_this());
        }
    }
}

void Entity::Tick(float deltaTime)
{
    if (auto ownerRoom = _room.load().lock())
    {
        ownerRoom->DoTimer(ENTITY_TICK_INTERVAL, &Room::TickEntity, shared_from_this());
    }
}

