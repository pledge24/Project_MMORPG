#include "Game/Combat/P1AnimNotify_AttackHit.h"

UP1AnimNotify_AttackHit::UP1AnimNotify_AttackHit()
{
#if WITH_EDITORONLY_DATA
    // 몽타주 편집기에서 다른 노티파이와 구별되게 칠한다.
    NotifyColor = FColor(220, 40, 40);
#endif
}

FString UP1AnimNotify_AttackHit::GetNotifyName_Implementation() const
{
    return TEXT("AttackHit");
}
