#include "pch.h"
#include "Combat.h"
#include "Creature.h"
#include "Player.h"
#include "Monster.h"

optional<Combat::HitResult> Combat::ResolveHit(const EntityRef& attacker, const CreatureRef& target, const Protocol::AttackInfo& attackInfo)
{
    // 공격은 판정을 뒤로 미뤄 예약된다. 그사이 공격자가 사망했으면 공격은 없던 것이 된다.
    if (CreatureRef attackerCreature = dynamic_pointer_cast<Creature>(attacker); attackerCreature && attackerCreature->IsDead())
        return nullopt;

    // 사망한 플레이어는 리스폰할 때까지 룸에 남으므로, 사망한 대상을 여기서 거른다.
    if (target->IsDead())
        return nullopt;

    target->OnHit(attacker, attackInfo);

    HitResult result;
    {
        result.updatedHp = target->GetStatValue(Protocol::STAT_TYPE_HP);
        result.isDead = target->IsDead();
    }

    PlayerRef player = dynamic_pointer_cast<Player>(attacker);
    MonsterRef monster = dynamic_pointer_cast<Monster>(target);
    if (result.isDead && player && monster)
    {
        KillResult kill;
        {
            kill.killer = player;
            kill.reward.set_exp(monster->GetExpReward());
            kill.reward.set_gold(monster->GetGoldReward());
        }
        result.kill = std::move(kill);
    }

    return result;
}
