#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "P1AnimNotify_ComboWindow.generated.h"

/**
 * 공격 몽타주에서 콤보의 다음 단계 입력을 받기 시작하는 순간을 표시하는 노티파이.
 *
 * 클라이언트의 콤보 처리에는 범용 몽타주 노티파이(`PlayMontageNotify`)와 같은 입력 창이고,
 * 입력 창 시점을 다른 노티파이와 구별하는 전용 타입이다.
 */
UCLASS(meta = (DisplayName = "P1 Combo Window"))
class P1_API UP1AnimNotify_ComboWindow : public UAnimNotify
{
    GENERATED_BODY()

public:
    UP1AnimNotify_ComboWindow();

    //~ Begin UAnimNotify Interface
public:
    virtual void BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload) override;
    virtual FString GetNotifyName_Implementation() const override;
#if WITH_EDITOR
    virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override;
#endif
    //~ End UAnimNotify Interface
};
