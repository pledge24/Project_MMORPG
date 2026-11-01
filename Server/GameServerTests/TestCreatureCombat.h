#pragma once
#include "Game/Data/Templates.h"
#include "Game/Combat/AttackArea.h"

/**
 * 테스트가 몬스터 표의 몬스터에 크리처 판정 데이터와 일반 공격 액션을 함께 넣는다.
 * 몬스터는 둘 중 하나라도 없으면 만들어지지 않는다. 실제 데이터처럼 일반 공격의 타격은 하나다.
 */
namespace TestCreatureCombat
{
    /** 첫 타격 시점이 hitTime초인 액션 타이밍. */
    inline ActionTiming MakeTiming(float hitTime)
    {
        ActionTiming timing;
        timing.montage = "/Game/Test/AM_TestAttack.AM_TestAttack";
        timing.section = "Default";
        timing.length = hitTime + 0.5f;
        timing.hitTimes = { hitTime };
        return timing;
    }

    /** 공격자 앞으로 radius까지 닿는 원. 높이는 공격자 발밑에서 위로 200이다. */
    inline DesignData::ActionHitRow MakeCircleHit(float radius)
    {
        DesignData::ActionHitRow hit;
        hit.HitIndex = 0;
        hit.Shape = DesignData::AttackAreaShape::CIRCLE;
        hit.Radius = radius;
        hit.ZMin = 0.f;
        hit.ZMax = 200.f;
        return hit;
    }

    /** 공격자 앞을 가운데에 둔 전체 각도 angle의 부채꼴. 높이는 공격자 발밑에서 위로 200이다. */
    inline DesignData::ActionHitRow MakeSectorHit(float radius, float angle)
    {
        DesignData::ActionHitRow hit = MakeCircleHit(radius);
        hit.Shape = DesignData::AttackAreaShape::SECTOR;
        hit.Angle = angle;
        return hit;
    }

    /** 적을 맞히는 물리 액션. 피해량은 공격자의 물리 공격력 × 계수다. */
    inline DesignData::ActionRow MakeAction(int32 actionId, const DesignData::ActionHitRow& hit, float coefficient = 1.f)
    {
        DesignData::ActionRow action;
        action.Id = actionId;
        action.DamageType = DesignData::DamageType::PHYSICAL;
        action.DamageCoefficient = coefficient;
        action.TargetRule = DesignData::ActionTarget::ENEMY;
        action.Hits = { hit };
        return action;
    }

    /** 몬스터 번호에 더해 그 몬스터의 일반 공격 액션 번호를 만든다. 몬스터 번호와 겹치지 않아야 서버가 액션 번호로 찾는지 드러난다. */
    constexpr int32 ACTION_ID_OFFSET = 100000;

    /** 몬스터와 플레이어의 캡슐. 실제 추출 값에 가깝다. */
    constexpr float MONSTER_CAPSULE_RADIUS = 34.f;
    constexpr float MONSTER_CAPSULE_HALF_HEIGHT = 88.f;
    constexpr float PLAYER_CAPSULE_RADIUS = 42.f;
    constexpr float PLAYER_CAPSULE_HALF_HEIGHT = 96.f;

    /**
     * 몬스터 표에 먼저 넣은 templateId 몬스터의 판정 데이터와 일반 공격 액션을 넣는다. 액션은 몬스터마다 따로 두려고
     * 그 몬스터의 NormalAttackAction을 templateId + ACTION_ID_OFFSET으로 바꾼다. 공격 범위를 주지 않으면 반지름 200인 원이다.
     */
    inline void AddMonster(GamedataTables& tables, int32 templateId, float hitTime = 0.2f,
        const DesignData::ActionHitRow& hit = MakeCircleHit(200.f))
    {
        CreatureCombat combat;
        combat.capsuleRadius = MONSTER_CAPSULE_RADIUS;
        combat.capsuleHalfHeight = MONSTER_CAPSULE_HALF_HEIGHT;
        tables.monsterCombats[templateId] = combat;

        const int32 actionId = templateId + ACTION_ID_OFFSET;
        tables.monsters.at(templateId).NormalAttackAction = actionId;
        tables.actions[actionId] = MakeAction(actionId, hit);
        tables.actionTimings[actionId] = MakeTiming(hitTime);
    }

    /** 대상 자리에 두는 시험용 액션. 액션 요청과 콤보를 거치지 않으려고 테스트가 룸의 타격 판정을 직접 부른다. */
    constexpr int32 STRIKE_ACTION_ID = 900000;

    /** 피해량이 공격자의 물리 공격력 × coefficient인 시험용 액션을 넣는다. 범위는 기준점 둘레 반지름 radius이고 높이는 넉넉하다. */
    inline void AddStrikeAction(GamedataTables& tables, float coefficient, float radius = 50.f)
    {
        DesignData::ActionHitRow hit = MakeCircleHit(radius);
        hit.ZMin = -1000.f;
        hit.ZMax = 1000.f;
        tables.actions[STRIKE_ACTION_ID] = MakeAction(STRIKE_ACTION_ID, hit, coefficient);
    }

    /** target이 선 자리를 기준점으로 둔 공격자 자세. 시험용 액션이 target을 맞힌다. */
    inline AttackerPose StrikePoseAt(const Protocol::PosInfo& target)
    {
        const Protocol::Vector& pos = target.pos();
        return AttackerPose{ vector3D(pos.x(), pos.y(), pos.z()), 0.f };
    }

    /** 그 직업의 크리처 판정 데이터(캡슐)를 넣는다. 없으면 그 직업의 플레이어는 캡슐 없이 점으로 판정된다. */
    inline void AddClass(GamedataTables& tables, Protocol::CharacterClass classId)
    {
        CreatureCombat combat;
        combat.capsuleRadius = PLAYER_CAPSULE_RADIUS;
        combat.capsuleHalfHeight = PLAYER_CAPSULE_HALF_HEIGHT;
        tables.classCombats[classId] = combat;
    }
}
