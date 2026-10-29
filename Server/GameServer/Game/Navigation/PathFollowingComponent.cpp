#include "Core/pch.h"
#include "Game/Navigation/PathFollowingComponent.h"
#include "Game/Entities/Creature.h"
#include "Game/Navigation/NavQuery.h"
#include "Game/Room/Room.h"

namespace
{
    float PlaneDistance(const vector3D& a, const vector3D& b)
    {
        return MathUtil::Distance(vector2D(a.x, a.y), vector2D(b.x, b.y));
    }
}

PathFollowingComponent::PathFollowingComponent(CreatureRef owner, float speed, float halfHeight)
    : EntityComponent(owner)
    , _speed(speed)
    , _halfHeight(halfHeight)
{
}

void PathFollowingComponent::Tick(float deltaTime)
{
    CreatureRef owner = GetOwner();
    if (owner == nullptr || IsMoving() == false)
        return;

    // 이번 틱에 갈 거리만큼 꺾이는 점을 차례로 지난다.
    const vector3D from = MathUtil::PosInfoToVector3D(owner->GetPosInfo());
    vector3D pos = from;
    float remaining = _speed * deltaTime;
    while (remaining > 0.f && _path.empty() == false)
    {
        const vector3D& next = _path.front();
        const float distance = PlaneDistance(pos, next);
        if (distance <= remaining)
        {
            pos = next;
            remaining -= distance;
            _path.pop_front();
            continue;
        }

        const float ratio = remaining / distance;
        pos = vector3D(pos.x + (next.x - pos.x) * ratio, pos.y + (next.y - pos.y) * ratio, pos.z + (next.z - pos.z) * ratio);
        remaining = 0.f;
    }

    owner->SetLocation(pos);
    const vector2D moved(pos.x - from.x, pos.y - from.y);
    if (moved != vector2D::GetZeroVector())
    {
        owner->SetYaw(MathUtil::VectorToYaw(moved));
        owner->SetMoveDirection(moved);
    }
    owner->SetMoveState(Protocol::MoveState::MOVE_STATE_RUN);

    if (_path.empty() || PlaneDistance(pos, vector3D(_goal.x, _goal.y, 0.f)) <= _acceptanceRadius)
        Stop();
}

bool PathFollowingComponent::MoveTo(const vector3D& dest, float acceptanceRadius)
{
    CreatureRef owner = GetOwner();
    if (owner == nullptr)
        return false;

    _path.clear();
    _goal = vector2D(dest.x, dest.y);
    _acceptanceRadius = acceptanceRadius;

    const vector3D from = MathUtil::PosInfoToVector3D(owner->GetPosInfo());
    if (PlaneDistance(from, dest) <= acceptanceRadius)
    {
        Stop();
        return true;
    }

    RoomRef room = owner->GetRoom();
    NavQuery* navQuery = room != nullptr ? room->GetNavQuery() : nullptr;
    if (navQuery == nullptr)
    {
        // 지형을 모르는 룸(테스트)이다. 높이는 지금 높이를 유지한다.
        _path.push_back(vector3D(dest.x, dest.y, from.z));
        return true;
    }

    // navmesh는 플레이어 캡슐의 중심 높이로 묻고 답한다. 소유자의 캡슐은 크기가 달라 높이를 옮겨 묻는다.
    const float heightOffset = _halfHeight - navQuery->GetAgentHalfHeight();
    const vector<vector3D> corners = navQuery->FindPath(
        vector3D(from.x, from.y, from.z - heightOffset), vector3D(dest.x, dest.y, dest.z - heightOffset));
    if (corners.empty())
    {
        Stop();
        return false;
    }

    // 첫 점은 출발점이 놓인 navmesh 위의 점이다.
    for (size_t i = 1; i < corners.size(); ++i)
        _path.push_back(vector3D(corners[i].x, corners[i].y, corners[i].z + heightOffset));

    if (_path.empty())
        Stop();
    return true;
}

void PathFollowingComponent::Stop()
{
    _path.clear();

    CreatureRef owner = GetOwner();
    if (owner == nullptr)
        return;

    owner->SetMoveDirection(vector2D::GetZeroVector());
    if (owner->GetPosInfo().state() == Protocol::MoveState::MOVE_STATE_RUN)
        owner->SetMoveState(Protocol::MoveState::MOVE_STATE_IDLE);
}

CreatureRef PathFollowingComponent::GetOwner() const
{
    // 이 컴포넌트를 만드는 엔티티는 Creature뿐이라 static_pointer_cast를 쓴다. 다른 엔티티에 붙이면 잘못된 형변환으로 메모리를 깨뜨린다.
    return static_pointer_cast<Creature>(_owner.lock());
}
