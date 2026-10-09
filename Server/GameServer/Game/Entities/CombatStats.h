#pragma once

/**
 * 최대 HP, 최대 MP, 물리 공격력, 마법 공격력 네 값.
 * 레벨 표의 기본 스탯과 장비가 올려 주는 증감량이 같은 모양이라 둘 다 이 구조체로 나타낸다.
 */
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
