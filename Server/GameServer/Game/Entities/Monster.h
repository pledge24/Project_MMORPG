#pragma once
#include "Game/Entities/Creature.h"

class TickIntervalTimer;
class TickTimer;

/** 몬스터 AI의 상태. StateCount는 상태 개수를 나타낼 뿐 상태가 아니다. */
enum class MonsterState : uint8
{
    Idle = 0,
    Wandering,
    Chasing,
    Attacking,
    Death,
    StateCount
};

/** 몬스터의 스폰 매개변수. spawnPos는 위치와 함께 배회의 기준점이 된다. */
struct MonsterSpawnParams : public Creature::SpawnParams
{
    int32 templateId = 0;
    Protocol::PosInfo spawnPos;
};

/**
 * 게임 기획 데이터의 몬스터 표로 만드는 AI 크리처.
 * Room::SpawnEntity가 만들어 룸에 넣고, 모든 처리가 그 룸 큐 위에서 돈다.
 * 이동과 행동은 룸 틱마다, 상태 전환 판정은 누적 시간이 UPDATE_STATE_INTERVAL을 채울 때마다 돈다.
 * 룸의 _entities가 붙잡고, 사망하면 룸이 빼낸다. 빠지면 룸 틱을 받지 않으므로 예약해 둔 피격도 사라진다.
 */
class Monster : public Creature
{
public:
    using SpawnParams = MonsterSpawnParams;

	Monster();
	virtual ~Monster();

protected:
    friend class EntityFactory;
    /** 몬스터 표에 없는 templateId면 false. */
    bool Init(const SpawnParams& params);
    virtual void Start() override;
    /** 예약한 피격, 상태 전환 판정(주기가 찼을 때), 상태별 행동 순서로 돈다. */
    virtual void Tick(float deltaTime) override;

public:
    //~ 이벤트
    /** 스폰 정보에 실리는 HP 사본(monster_info.hp)도 함께 맞춘다. */
    virtual void OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo) override;
    virtual void OnDie(EntityRef attacker) override;

    //~ Getter
    int64 GetTemplateId() { return _templateId; }
    /** 부를 때마다 표의 최솟값과 최댓값 사이에서 새로 뽑는다. */
    int64 GetExpReward();
    /** 부를 때마다 표의 최솟값과 최댓값 사이에서 새로 뽑는다. */
    int64 GetGoldReward();

protected:
    //~ 상태
    void EvaluateStateTransition();
    /** _stateTimer를 0으로 돌리고 새 상태의 진입 처리(목적지, 방향, 이동 상태)를 한다. */
    void SwitchState(MonsterState nextState);

    /** 상태별 행동을 틱마다 실행한다. deltaTime은 초 단위다. */
    void ExecuteStateBehavior(float deltaTime);
    void ExecuteStateNone();
    void ExecuteStateIdle(float deltaTime);
    void ExecuteStateWandering(float deltaTime);
    void ExecuteStateAttacking(float deltaTime);
    void ExecuteStateChasing(float deltaTime);
    void ExecuteStateDeath(float deltaTime);

    //~ AI
    /** 목적지가 없거나 이미 도착했으면 이동하지 않고 멈춘다. deltaTime은 초 단위다. */
    void Move(float deltaTime, bool orientRotationToMovement = true);
    void LookAt(const vector2D& targetPos);
    void StartMovingTo(const vector2D& dest, float minApproachDistance = 0.f);
    /** context는 지금 쓰지 않는다. shouldBeIdle이면 달리던 중이 아니어도 Idle 이동 상태로 바꾼다. */
    void StopMoving(string context = "", bool shouldBeIdle = false);
    /** 룸에 일반 공격을 알리고, NORMAL_ATTACK_HIT_DELAY 뒤의 피격 판정을 예약한다. */
    void NormalAttack();
    /** 예약한 피격의 남은 시간을 줄이고, 다 됐으면 Room::HandleHit에 넘긴다. */
    void UpdatePendingHit(float deltaTime);

    //~ 상태 확인
    /** 대상이 사라졌거나, 사망했거나, 이 룸을 떠났으면 true. */
    bool IsTargetLost();
    /** 목적지가 있고 마지막 공격 뒤 _attackInterval이 지났을 때만 true. */
    bool CanMove();
    /** 목적지가 없어도 true다. */
    bool AlreadyArrive();
    bool HasDestination() { return _moveDest.has_value(); }
    /** 지금은 일반 공격만 데이터의 IsTargeting을 따르고, 나머지는 false다. */
    bool IsTargetingAttack(Protocol::AttackType type);

    //~ Getter
    /** 목적지가 없으면 부르지 않는다. HasDestination으로 먼저 확인한다. */
    const vector2D& GetDestination() { return _moveDest.value(); }

    //~ Setter
    /** minApproachDistance가 0보다 크면 목적지에서 그만큼 덜 간 지점을 목적지로 잡는다. 이미 그보다 가까우면 현재 위치다. */
    void SetDestination(const vector2D& destPos, float minApproachDistance = 0.f);
    void SetMoveDirection(const vector2D& moveVec);
    void SetYaw(float yaw);

    //~ 기타
    void ClearDestination();
    void PrintMonsterAllData() const;
    /** _template에서 스탯과 AI 값을 읽어 멤버에 둔다. */
    void CacheMonsterData();

private:
    //~ Monster Template
    /** 몬스터 표의 행 사본. 보상은 여기서 뽑는다. */
    MonsterTemplate _template;

    //~ Monster Stat Data
    /** _entityInfo 안의 monster_info를 가리킨다. 따로 지우지 않는다. */
    Protocol::MonsterInfo* _monsterInfo;
    int32 _templateId;
    int32 _maxHp;
    /** 공격 간격(초). */
    float _attackInterval;
    int32 _baseAttack;

    //~ Monster AI Data(Common)
    /** 단위는 초다. */
    static constexpr float IDLE_TIME = 5.f;
    /** 단위는 초다. */
    static constexpr float WANDERING_TIME = 2.f;
    /** 상태 전환 판정의 주기. 단위는 초다. */
    static constexpr float UPDATE_STATE_INTERVAL = 0.2f;
    /** 일반 공격을 알린 뒤 피격을 판정하기까지의 시간. 단위는 초다. */
    static constexpr float NORMAL_ATTACK_HIT_DELAY = 0.2f;
    static constexpr float MIN_APPROACH_DISTANCE = 120.f;

    //~ Monster AI Data(Individual)
    MonsterState _state = MonsterState::Idle;
    vector2D _spawnPos;
    float _tryAttackRange;                      // 공격 사거리
    float _detectionRange;                      // 타겟 감지 범위
    float _chasingMaxRange;                     // 추적 범위
    float _monsterSpeed;                        // 몬스터 이동 속도
    bool _isTargeting;

    weak_ptr<Entity> _target;
    optional<vector2D> _moveDest;

    //~ Timer
    /** 아래 세 타이머의 단위는 초다. */
    float _stateTimer = 0.f;                    // 여러 용도로 사용됨
    float _timeSinceLastAttack = 0.f;
    float _timeSinceStateUpdate = 0.f;

    /** 판정을 기다리는 일반 공격. remainingTime은 초 단위다. */
    struct PendingHit
    {
        Protocol::AttackInfo attackInfo;
        float remainingTime = 0.f;
    };
    optional<PendingHit> _pendingHit;

    TickTimer* _attackTimer = nullptr;          // 사용 안하는 중
};

