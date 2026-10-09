#include "Core/pch.h"
#include "Game/Entities/Entity.h"
#include "Game/Room/Room.h"

Entity::Entity()
    : _entityInfo(make_unique<Protocol::EntityInfo>())
{
    _posInfo = _entityInfo->mutable_pos_info();
}

Entity::~Entity()
{
}

void Entity::JoinRoom(const RoomRef& room)
{
    _room.store(room);

    if (_hasBegunPlay == false)
    {
        _hasBegunPlay = true;
        Start();
    }
}

bool Entity::Init(const SpawnParams&)
{
    return true;
}

