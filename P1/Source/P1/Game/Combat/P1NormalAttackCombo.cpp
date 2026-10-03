#include "Game/Combat/P1NormalAttackCombo.h"

int32 FP1NormalAttackCombo::Next(int32 Current, int32 MontageCount)
{
    if (MontageCount <= 0)
        return 0;

    return (Current % MontageCount) + 1;
}

int32 FP1NormalAttackCombo::MontageIndexFor(int32 Combo, int32 MontageCount)
{
    // 몬스터 공격은 서버가 순번 0을 보낸다. 첫 몽타주로 본다.
    const int32 Index = Combo == 0 ? 0 : Combo - 1;
    if (Index < 0 || Index >= MontageCount)
        return INDEX_NONE;

    return Index;
}
