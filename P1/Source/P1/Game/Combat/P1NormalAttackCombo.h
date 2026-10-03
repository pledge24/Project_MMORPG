#pragma once

#include "CoreMinimal.h"

/**
 * 일반 공격의 콤보 순번 규칙이다. 순번은 1부터 몽타주 수까지 돌고, 0은 콤보가 없는 상태다.
 */
struct P1_API FP1NormalAttackCombo
{
    /** 지금 순번 다음에 올 순번이다. 몽타주가 없으면 0이다. */
    static int32 Next(int32 Current, int32 MontageCount);

    /** 순번에 맞는 몽타주의 인덱스다. 0은 첫 몽타주로 본다. 범위를 벗어나거나 몽타주가 없으면 INDEX_NONE이다. */
    static int32 MontageIndexFor(int32 Combo, int32 MontageCount);
};
