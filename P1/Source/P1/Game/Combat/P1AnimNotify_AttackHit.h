#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "P1AnimNotify_AttackHit.generated.h"

/**
 * 공격 몽타주에서 공격이 대상에게 맞는 순간을 표시하는 노티파이.
 *
 * 클라이언트에서는 아무 일도 하지 않는 표시이고, 이 시점이 서버가 피격을 판정하는 시점이 된다.
 */
UCLASS(meta = (DisplayName = "P1 Attack Hit"))
class P1_API UP1AnimNotify_AttackHit : public UAnimNotify
{
    GENERATED_BODY()

public:
    UP1AnimNotify_AttackHit();

    //~ Begin UAnimNotify Interface
public:
    virtual FString GetNotifyName_Implementation() const override;
    //~ End UAnimNotify Interface
};
