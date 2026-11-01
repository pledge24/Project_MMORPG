#pragma once

/** 타격 하나가 피격 대상에게 주는 것. */
struct AttackInfo
{
    int64 damage = 0;
};

/** 액션이 어느 공격력으로 피해를 주는지. */
enum class DamageType : uint8
{
    Physical,
    Magical,
};

/** 처치 한 번의 보상. */
struct Reward
{
    int64 exp = 0;
    int64 gold = 0;
};
