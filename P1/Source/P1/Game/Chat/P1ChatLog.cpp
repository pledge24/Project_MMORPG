#include "Game/Chat/P1ChatLog.h"

FP1ChatLog::FP1ChatLog(int32 InCapacity)
    : Capacity(FMath::Max(1, InCapacity))
{
}

void FP1ChatLog::Add(FP1ChatLine Line)
{
    if (Lines.Num() >= Capacity)
        Lines.RemoveAt(0, Lines.Num() - Capacity + 1);

    Lines.Add(MoveTemp(Line));
}

void FP1ChatLog::Clear()
{
    Lines.Empty();
}
