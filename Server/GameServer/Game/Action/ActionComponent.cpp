#include "Core/pch.h"
#include "Game/Action/ActionComponent.h"
#include "Game/Combat/AttackArea.h"
#include "Game/Entities/Creature.h"
#include "Game/Room/Room.h"

ActionComponent::ActionComponent(CreatureRef owner)
    : EntityComponent(owner)
{
}

void ActionComponent::Tick(float deltaTime)
{
    if (_running.has_value() == false)
        return;

    _running->elapsed += deltaTime;

    // 틱이 길면 타격 여럿이 한 틱에 지난다. 타격 순번 순서로 모두 판정한다.
    // 판정이 이 크리처의 액션을 끝낼 수 있으므로(룸에서 빠짐) 판정마다 진행 중인지 다시 본다.
    while (_running.has_value() && _running->nextHitIndex < static_cast<int32>(_running->hitTimes.size())
        && _running->elapsed >= _running->hitTimes[_running->nextHitIndex])
    {
        const int32 hitIndex = _running->nextHitIndex++;
        JudgeHit(hitIndex);
    }

    if (_running.has_value() && _running->elapsed >= _running->length)
        _running.reset();
}

bool ActionComponent::StartAction(int32 actionId, float yaw)
{
    CreatureRef owner = GetOwner();
    RoomRef ownerRoom = owner ? owner->GetRoom() : nullptr;
    if (ownerRoom == nullptr)
        return false;

    // 기동할 때 액션 표의 액션마다 판정 데이터가 있는지, 타격 시점의 수가 타격 수와 같은지 대조했다. 없으면 표를 바꿔 끼운 테스트다.
    const ActionTiming* timing = Gamedata::FindActionTiming(actionId);
    if (Gamedata::FindAction(actionId) == nullptr || timing == nullptr)
    {
        GLogger->Warning("크리처 {}: 액션 표나 크리처 판정 데이터에 없는 액션 {}을 시작할 수 없다", owner->GetEntityId(), actionId);
        return false;
    }

    RunningAction running;
    {
        running.actionId = actionId;
        running.yaw = yaw;
        running.length = timing->length;
        running.hitTimes = timing->hitTimes;
    }
    _running = std::move(running);

    ownerRoom->HandleActionStart(owner, actionId, yaw);
    return true;
}

void ActionComponent::JudgeHit(int32 hitIndex)
{
    CreatureRef owner = GetOwner();
    RoomRef ownerRoom = owner ? owner->GetRoom() : nullptr;
    if (ownerRoom == nullptr)
        return;

    // 위치의 z는 캡슐 중심이고 공격 범위의 높이는 발밑 기준이다.
    const Protocol::Vector& pos = owner->GetPosInfo().pos();
    AttackerPose pose;
    {
        pose.foot = vector3D(pos.x(), pos.y(), pos.z() - owner->GetCapsule().capsuleHalfHeight);
        pose.yaw = _running->yaw;
    }

    ownerRoom->HandleHit(owner, _running->actionId, hitIndex, pose);
}

CreatureRef ActionComponent::GetOwner() const
{
    // 이 컴포넌트를 만드는 엔티티는 Creature뿐이라 static_pointer_cast를 쓴다. 다른 엔티티에 붙이면 잘못된 형변환으로 메모리를 깨뜨린다.
    return static_pointer_cast<Creature>(_owner.lock());
}
