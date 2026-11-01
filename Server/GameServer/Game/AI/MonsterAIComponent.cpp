#include "Core/pch.h"
#include "Game/AI/MonsterAIComponent.h"
#include "Game/Action/ActionComponent.h"
#include "Game/Entities/Monster.h"
#include "Game/Entities/Player.h"
#include "Game/Navigation/PathFollowingComponent.h"
#include "Game/Room/Room.h"

MonsterAIComponent::MonsterAIComponent(MonsterRef owner, const DesignData::MonsterRow& monsterRow)
    : EntityComponent(owner)
{
    _attackInterval = monsterRow.AttackInterval;
    _normalAttackAction = monsterRow.NormalAttackAction;
    _tryAttackRange = monsterRow.TryAttackRange;
    _detectionRange = monsterRow.DetectionRange;
    _chasingMaxRange = monsterRow.ChasingMaxRange;
    _timeSinceLastAttack = _attackInterval;

    // 스폰 정보에 Idle 이동 상태가 실리도록 룸에 들어가기 전에 맞춰 둔다.
    _state = &MonsterState::Get(MonsterStateType::Idle);
    _state->Enter(*this);
}

void MonsterAIComponent::Tick(float deltaTime)
{
    // 타이머 갱신.
    {
        _stateTimer += deltaTime;
        _timeSinceLastAttack += deltaTime;
        _timeSinceStateUpdate += deltaTime;
    }

    // 대상의 위치는 대상이 이 룸에 있을 때만 읽을 수 있다. 다른 룸으로 떠난 대상의 위치는 그 룸 큐가 쓴다.
    // 그래서 대상 확인은 상태 갱신 주기를 기다리지 않고 틱마다 하고, 놓친 대상은 쫓지 않고 스폰 지점으로 돌아간다.
    if (_target.expired() == false && IsTargetLost())
        SwitchState(MonsterStateType::Returning);

    // 상태 갱신은 틱보다 긴 주기로 돈다. 남은 시간을 넘기지 않고 0으로 돌리므로 주기가 틱 간격만큼 늦어질 수 있다.
    if (_timeSinceStateUpdate >= UPDATE_STATE_INTERVAL)
    {
        _timeSinceStateUpdate = 0.f;
        if (optional<MonsterStateType> nextState = _state->Update(*this))
            SwitchState(nextState.value());
    }

    // 상태별 틱 행동.
    _state->Tick(*this, deltaTime);
}

void MonsterAIComponent::SwitchState(MonsterStateType nextState)
{
    _state->Exit(*this);
    _state = &MonsterState::Get(nextState);
    _stateTimer = 0.f;
    _state->Enter(*this);
}

bool MonsterAIComponent::IsTargetLost() const
{
    // 사망한 플레이어는 리스폰할 때까지 룸에 남으므로, 룸 소속만 보지 않고 사망도 따로 본다.
    EntityRef target = _target.lock();
    if (target == nullptr)
        return true;

    // 룸을 먼저 본다. 떠난 대상의 사망 표시는 그 대상이 들어간 룸 큐가 쓴다.
    MonsterRef owner = GetOwner();
    RoomRef ownerRoom = owner ? owner->GetRoom() : nullptr;
    if (ownerRoom == nullptr || ownerRoom->Contains(target->GetEntityId()) == false)
        return true;

    CreatureRef creature = dynamic_pointer_cast<Creature>(target);
    return creature && creature->IsDead();
}

bool MonsterAIComponent::FindTarget()
{
    MonsterRef owner = GetOwner();
    RoomRef ownerRoom = owner ? owner->GetRoom() : nullptr;
    if (ownerRoom == nullptr)
        return false;

    PlayerRef player = ownerRoom->FindClosestPlayer(&owner->GetPosInfo(), _detectionRange).first;
    if (player == nullptr)
        return false;

    _target = static_pointer_cast<Entity>(player);
    return true;
}

bool MonsterAIComponent::IsTargetInAttackRange() const
{
    MonsterRef owner = GetOwner();
    const Protocol::PosInfo* targetPos = GetTargetPos();
    return owner != nullptr && targetPos != nullptr && MathUtil::InRange(&owner->GetPosInfo(), targetPos, _tryAttackRange);
}

void MonsterAIComponent::LookAtTarget()
{
    // 공격은 시작한 방향으로 판정한다. 대상을 따라 돌면 클라이언트가 보여 주는 방향과 판정이 어긋난다.
    if (IsAttacking())
        return;

    FaceTarget();
}

bool MonsterAIComponent::IsBeyondChaseLimit() const
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return false;

    const vector3D& spawnPos = owner->GetSpawnPoint().pos;
    const vector2D pos = MathUtil::PosInfoToVector2D(&owner->GetPosInfo());
    return MathUtil::Distance(pos, vector2D(spawnPos.x, spawnPos.y), true) > _chasingMaxRange * _chasingMaxRange;
}

void MonsterAIComponent::ChaseTarget()
{
    MonsterRef owner = GetOwner();
    const Protocol::PosInfo* targetPos = GetTargetPos();
    if (owner == nullptr || targetPos == nullptr)
        return;

    // 공격한 뒤 공격 간격 동안은 공격 몽타주가 재생 중이라 움직이지 않는다. 판정은 타격 순간의 위치로 하므로,
    // 공격 간격이 액션보다 짧아도 공격하는 동안 움직이면 범위 밖으로 피한 대상을 따라가 맞힌다.
    if (CanAttack() == false || IsAttacking())
        return;

    owner->GetPathFollowing().MoveTo(MathUtil::PosInfoToVector3D(*targetPos), MIN_APPROACH_DISTANCE);
}

void MonsterAIComponent::WanderInSpawnRadius()
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    // 마커의 z는 바닥이다. 목적지는 그 바닥에 선 캡슐의 중심 높이로 준다.
    const SpawnPointPlacement& spawnPoint = owner->GetSpawnPoint();
    PathFollowingComponent& pathFollowing = owner->GetPathFollowing();
    const vector2D dest = Room::GetRandomLocation(vector2D(spawnPoint.pos.x, spawnPoint.pos.y), spawnPoint.radius);
    pathFollowing.MoveTo(vector3D(dest.x, dest.y, spawnPoint.pos.z + pathFollowing.GetHalfHeight()));
}

void MonsterAIComponent::ReturnToSpawnPoint()
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    // 길을 찾지 못하면 멈춘 자리에서 귀환을 마친다.
    const SpawnPointPlacement& spawnPoint = owner->GetSpawnPoint();
    PathFollowingComponent& pathFollowing = owner->GetPathFollowing();
    pathFollowing.MoveTo(vector3D(spawnPoint.pos.x, spawnPoint.pos.y, spawnPoint.pos.z + pathFollowing.GetHalfHeight()), spawnPoint.radius);
}

bool MonsterAIComponent::IsMoving() const
{
    MonsterRef owner = GetOwner();
    return owner != nullptr && owner->GetPathFollowing().IsMoving();
}

void MonsterAIComponent::StopMoving()
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    owner->GetPathFollowing().Stop();
    owner->SetMoveState(Protocol::MoveState::MOVE_STATE_IDLE);
}

void MonsterAIComponent::HoldForAttack()
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    owner->GetPathFollowing().Stop();
    LookAtTarget();
    owner->SetMoveState(Protocol::MoveState::MOVE_STATE_ACTION);
}

void MonsterAIComponent::NormalAttack()
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr || _target.expired())
        return;

    // 방향은 여기서 정해져 공격이 끝날 때까지 바뀌지 않는다. 플레이어는 그 방향을 보고 범위 밖으로 피한다.
    // 앞 공격이 끝나기 전에 새 공격을 시작해도 대상을 다시 바라본다. 타격은 대상을 정하지 않고, 타격 순간 공격 범위 안에 있는 플레이어가 맞는다.
    FaceTarget();
    if (owner->GetAction().StartAction(_normalAttackAction, owner->GetPosInfo().yaw()))
        _timeSinceLastAttack = 0.f;
}

void MonsterAIComponent::CancelAttack()
{
    if (MonsterRef owner = GetOwner())
        owner->GetAction().Cancel();
}

bool MonsterAIComponent::IsAttacking() const
{
    MonsterRef owner = GetOwner();
    return owner != nullptr && owner->GetAction().IsActing();
}

void MonsterAIComponent::RecoverHp(float deltaTime)
{
    MonsterRef owner = GetOwner();
    if (owner == nullptr)
        return;

    // 회복량을 정수로 내리면 틱마다 0이 될 수 있어 올린다. 귀환이 짧아도 조금은 회복한다.
    const int64 maxHp = owner->GetStatValue(Protocol::STAT_TYPE_MAX_HP);
    const int64 recovered = static_cast<int64>(ceilf(maxHp * RETURN_HP_RECOVERY_RATIO * deltaTime));
    const int64 hp = owner->GetStatValue(Protocol::STAT_TYPE_HP);
    owner->SetStatValue(Protocol::STAT_TYPE_HP, min(maxHp, hp + recovered));
}

void MonsterAIComponent::FaceTarget()
{
    MonsterRef owner = GetOwner();
    const Protocol::PosInfo* targetPos = GetTargetPos();
    if (owner == nullptr || targetPos == nullptr)
        return;

    const vector2D lookAtVec = MathUtil::PosInfoToVector2D(targetPos) - MathUtil::PosInfoToVector2D(&owner->GetPosInfo());
    if (lookAtVec != vector2D::GetZeroVector())
        owner->SetYaw(MathUtil::VectorToYaw(lookAtVec));
}

MonsterRef MonsterAIComponent::GetOwner() const
{
    // 이 컴포넌트를 만드는 엔티티는 Monster뿐이라 static_pointer_cast를 쓴다. 다른 엔티티에 붙이면 잘못된 형변환으로 메모리를 깨뜨린다.
    return static_pointer_cast<Monster>(_owner.lock());
}

const Protocol::PosInfo* MonsterAIComponent::GetTargetPos() const
{
    EntityRef target = _target.lock();
    return target != nullptr ? &target->GetPosInfo() : nullptr;
}
