#pragma once

/** 전투 스탯 네 값. 레벨 표의 기본 스탯과 장비의 증감량을 함께 나타낸다. */
struct CombatStats
{
    int64 maxHp = 0;
    int64 maxMp = 0;
    int64 physicalAttack = 0;
    int64 magicalAttack = 0;

    CombatStats& operator+=(const CombatStats& other)
    {
        maxHp += other.maxHp;
        maxMp += other.maxMp;
        physicalAttack += other.physicalAttack;
        magicalAttack += other.magicalAttack;
        return *this;
    }
};
