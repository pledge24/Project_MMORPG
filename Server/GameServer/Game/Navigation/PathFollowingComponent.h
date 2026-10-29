#pragma once
#include "Game/Entities/EntityComponent.h"

/**
 * 크리처를 목적지까지 navmesh의 길을 따라 옮기는 부품.
 *
 * 쓰는 쪽(AI)이 목적지만 정하면 소속 룸의 navmesh로 길을 찾고, 틱마다 꺾이는 점을 따라 크리처의 위치와 방향을 옮긴다.
 * 소속 룸에 navmesh가 없으면 목적지로 곧장 간다. 룸 큐 위에서만 쓴다.
 */
class PathFollowingComponent : public EntityComponent
{
public:
    /** speed는 초당 cm, halfHeight는 소유자 캡슐의 반높이(cm)다. */
    PathFollowingComponent(CreatureRef owner, float speed, float halfHeight);
    virtual ~PathFollowingComponent() = default;

    //~ Begin EntityComponent Interface
public:
    virtual void Tick(float deltaTime) override;
    //~ End EntityComponent Interface

    //~ 경로 추종
public:
    /**
     * dest에서 평면 거리 acceptanceRadius 안에 들 때까지 길을 따라 움직인다. dest의 z는 캡슐 중심이다.
     * 이미 들어 있으면 움직이지 않는다. 닿지 못하는 dest면 가장 가까이 가는 점까지 가고, 길을 찾지 못하면 멈추고 false.
     */
    bool MoveTo(const vector3D& dest, float acceptanceRadius = 0.f);
    /** 멈추고 남은 길을 버린다. 달리던 중이었으면 이동 상태를 Idle로 바꾼다. */
    void Stop();
    /** 따라갈 길이 남아 있으면 true. 도착했거나 멈췄으면 false. */
    bool IsMoving() const { return _path.empty() == false; }

private:
    /** 소유자가 이미 사라졌으면 nullptr. */
    CreatureRef GetOwner() const;

    /** 초당 cm. */
    float _speed;
    /** 소유자 캡슐의 반높이(cm). navmesh의 꺾이는 점은 플레이어 캡슐의 중심 높이라 이 값으로 옮긴다. */
    float _halfHeight;
    /** 남은 꺾이는 점. 앞이 다음 점이고, z는 소유자 캡슐의 중심이다. */
    deque<vector3D> _path;
    vector2D _goal;
    float _acceptanceRadius = 0.f;
};
