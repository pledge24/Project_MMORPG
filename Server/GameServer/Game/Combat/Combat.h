#pragma once

/**
 * 피격과 처치의 판정.
 *
 * 공격 하나가 대상에게 미치는 결과를 정하는 게임 규칙만 맡는다.
 * 대상을 찾고 결과를 알리는 일은 판정을 부르는 쪽의 몫이다.
 */
namespace Combat
{
    /** 처치는 플레이어의 공격으로 몬스터가 사망하는 것이다. */
    struct KillResult
    {
        PlayerRef killer;
        /** 몬스터 표의 범위에서 이번 처치마다 새로 뽑은 보상이다. */
        Protocol::Reward reward;
    };

    /** 피격 한 번의 결과. */
    struct HitResult
    {
        int64 updatedHp = 0;
        bool isDead = false;

        /** 처치일 때만 값이 있다. */
        optional<KillResult> kill;
    };

    /**
     * 피격을 적용하고 결과를 돌려준다. 대상을 바꾸므로 대상이 속한 룸 큐 위에서만 부른다.
     * 공격자나 대상이 이미 사망했으면 아무것도 바꾸지 않고 nullopt.
     */
    optional<HitResult> ResolveHit(const EntityRef& attacker, const CreatureRef& target, const Protocol::AttackInfo& attackInfo);
}
