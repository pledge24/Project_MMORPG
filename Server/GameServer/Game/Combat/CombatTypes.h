#pragma once

/** 공격 하나가 대상에게 주는 것. 판정을 미뤄 예약할 때 그대로 들고 간다. */
struct AttackInfo
{
    /** 비어 있으면 대상을 정하지 않은 공격이다. */
    optional<int64> targetId;
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
