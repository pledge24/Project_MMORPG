#pragma once

/*----------------------
        Combat
-----------------------*/
// 피격과 처치를 판정한다. 룸과 세션을 모른다.
// 대상 찾기와 결과 전송은 룸이 맡는다.
namespace Combat
{
    struct HitResult
    {
        CreatureRef target;
        int64 damage = 0;
        int64 updatedHp = 0;
        bool isDead = false;

        // 처치일 때만 값이 있다. 처치는 플레이어의 공격으로 몬스터가 사망하는 것이다.
        optional<Protocol::Reward> killReward;
    };

    // 피격을 적용하고 결과를 돌려준다. 공격자나 대상이 이미 사망했으면 아무것도 바꾸지 않고 nullopt.
    optional<HitResult> ResolveHit(const EntityRef& attacker, const CreatureRef& target, const Protocol::AttackInfo& attackInfo);
}
