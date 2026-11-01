#pragma once
#include "Game/Entities/EntityComponent.h"

/**
 * 크리처가 진행 중인 액션.
 *
 * 액션 하나를 시작해 끝날 때까지 시간을 세고, 타격 시점마다 룸에 타격 판정을 넘긴다. 룸 큐 위에서만 쓴다.
 */
class ActionComponent : public EntityComponent
{
public:
    explicit ActionComponent(CreatureRef owner);
    virtual ~ActionComponent() = default;

    //~ Begin EntityComponent Interface
public:
    virtual void Tick(float deltaTime) override;
    //~ End EntityComponent Interface

    //~ 액션
public:
    /**
     * 액션을 yaw(도) 방향으로 시작하고 주변에 알린다. 방향은 액션이 끝날 때까지 바뀌지 않는다. 진행 중인 액션은 버린다.
     * 룸 밖이거나 액션 표나 크리처 판정 데이터에 없는 액션이면 시작하지 않고 false.
     */
    bool StartAction(int32 actionId, float yaw);
    /** 진행 중인 액션과 아직 판정하지 않은 타격을 버린다. */
    void Cancel() { _running.reset(); }
    bool IsActing() const { return _running.has_value(); }

private:
    /** 타격 순번 hitIndex를 룸에 넘긴다. 기준 위치는 지금 위치의 발밑이다. */
    void JudgeHit(int32 hitIndex);

    /** 진행 중인 액션. 시간은 액션을 시작한 뒤 흐른 초다. */
    struct RunningAction
    {
        int32 actionId = 0;
        float yaw = 0.f;
        /** 판정 데이터의 사본. 섹션 길이와 타격 순번 순서의 타격 시점이다. */
        float length = 0.f;
        vector<float> hitTimes;

        float elapsed = 0.f;
        int32 nextHitIndex = 0;
    };
    optional<RunningAction> _running;

    //~ 공통
private:
    /** 소유 크리처. 소유자가 이미 사라졌으면 nullptr. */
    CreatureRef GetOwner() const;
};
