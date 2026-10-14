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

bool Entity::Init(const SpawnParams&)
{
    return true;
}

void Entity::JoinRoom(const RoomRef& room)
{
    // 룸을 Start보다 먼저 쓴다. 순서를 바꾸면 Start 안에서 읽는 소속 룸이 비어 있다.
    _room.store(room);

    if (_hasBegunPlay == false)
    {
        _hasBegunPlay = true;
        Start();
    }
}
