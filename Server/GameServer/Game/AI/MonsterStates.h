#pragma once

class MonsterAIComponent;

/** 몬스터 AI의 상태 종류. 사망한 몬스터는 룸에서 빠지므로 사망 상태를 두지 않는다. */
enum class MonsterStateType : uint8
{
    Idle = 0,
    Wandering,
    Chasing,
    Attacking,
    Returning,
};

/**
 * 몬스터 AI 상태 하나의 행동.
 *
 * 상태에 들어올 때, 상태 갱신 주기마다, 룸 틱마다, 상태를 떠날 때 할 일을 정한다. 다음 상태는 갱신이 돌려주고,
 * 상태를 바꾸는 일은 AI가 한다. 상태는 값을 갖지 않으므로 모든 몬스터가 같은 객체를 함께 쓴다.
 */
class MonsterState
{
public:
    virtual ~MonsterState() = default;

    /** 그 종류의 상태 객체. */
    static const MonsterState& Get(MonsterStateType type);

    virtual MonsterStateType GetType() const = 0;
    virtual void Enter(MonsterAIComponent& ai) const {}
    /** 상태 갱신 주기마다 부른다. 다른 상태로 바꿔야 하면 그 상태. */
    virtual optional<MonsterStateType> Update(MonsterAIComponent& ai) const = 0;
    /** 룸 틱마다 부른다. deltaTime은 초 단위다. */
    virtual void Tick(MonsterAIComponent& ai, float deltaTime) const {}
    virtual void Exit(MonsterAIComponent& ai) const {}
};

/** 멈춰 서서 감지 범위의 플레이어를 찾는다. 대기 시간이 지나면 배회한다. */
class MonsterIdleState final : public MonsterState
{
public:
    virtual MonsterStateType GetType() const override { return MonsterStateType::Idle; }
    virtual void Enter(MonsterAIComponent& ai) const override;
    virtual optional<MonsterStateType> Update(MonsterAIComponent& ai) const override;

private:
    /** 대기를 유지하는 시간(초). */
    static constexpr float IDLE_TIME = 5.f;
};

/** 스폰 반경 안의 무작위 지점으로 걸으며 감지 범위의 플레이어를 찾는다. */
class MonsterWanderingState final : public MonsterState
{
public:
    virtual MonsterStateType GetType() const override { return MonsterStateType::Wandering; }
    virtual void Enter(MonsterAIComponent& ai) const override;
    virtual optional<MonsterStateType> Update(MonsterAIComponent& ai) const override;

private:
    /** 배회를 유지하는 최대 시간(초). */
    static constexpr float WANDERING_TIME = 2.f;
};

/** 대상을 쫓는다. 스폰 지점에서 추적 한계보다 멀어지면 귀환한다. */
class MonsterChasingState final : public MonsterState
{
public:
    virtual MonsterStateType GetType() const override { return MonsterStateType::Chasing; }
    virtual void Enter(MonsterAIComponent& ai) const override;
    virtual optional<MonsterStateType> Update(MonsterAIComponent& ai) const override;
};

/** 멈춰 서서 대상을 바라보고 공격 간격마다 일반 공격을 한다. */
class MonsterAttackingState final : public MonsterState
{
public:
    virtual MonsterStateType GetType() const override { return MonsterStateType::Attacking; }
    virtual void Enter(MonsterAIComponent& ai) const override;
    virtual optional<MonsterStateType> Update(MonsterAIComponent& ai) const override;
    virtual void Tick(MonsterAIComponent& ai, float deltaTime) const override;
};

/** 대상을 놓고 스폰 지점으로 돌아간다. 돌아가는 동안은 대상을 잡지 않고 체력을 회복한다. */
class MonsterReturningState final : public MonsterState
{
public:
    virtual MonsterStateType GetType() const override { return MonsterStateType::Returning; }
    virtual void Enter(MonsterAIComponent& ai) const override;
    virtual optional<MonsterStateType> Update(MonsterAIComponent& ai) const override;
    virtual void Tick(MonsterAIComponent& ai, float deltaTime) const override;
};
