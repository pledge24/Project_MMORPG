#pragma once

#include "CoreMinimal.h"
#include "Game/Chat/P1ChatLine.h"

/**
 * 채팅창이 보관하는 최근 줄의 목록.
 *
 * 들어온 순서대로 줄을 보관하고, 정한 수를 넘으면 가장 오래된 줄부터 버린다.
 */
class P1_API FP1ChatLog
{
public:
    /** Capacity가 1보다 작으면 1로 본다. */
    explicit FP1ChatLog(int32 InCapacity);

    void Add(FP1ChatLine Line);
    void Clear();

    /** 오래된 줄이 앞에 있다. */
    const TArray<FP1ChatLine>& GetLines() const { return Lines; }
    int32 GetCapacity() const { return Capacity; }

private:
    int32 Capacity = 1;
    TArray<FP1ChatLine> Lines;
};
