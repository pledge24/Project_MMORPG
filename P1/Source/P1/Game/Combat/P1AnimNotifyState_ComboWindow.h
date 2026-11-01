#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "P1AnimNotifyState_ComboWindow.generated.h"

/**
 * 공격 몽타주에서 콤보의 다음 단계 입력을 받는 구간을 표시하는 노티파이 스테이트.
 *
 * 콤보의 다음 단계는 이 구간 안에서 누른 입력으로만 이어진다.
 * 클라이언트의 콤보 처리에는 범용 몽타주 노티파이 윈도우(`PlayMontageNotifyWindow`)와 같은 입력 창이다.
 */
UCLASS(meta = (DisplayName = "P1 Combo Window"))
class P1_API UP1AnimNotifyState_ComboWindow : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UP1AnimNotifyState_ComboWindow();

    //~ Begin UAnimNotifyState Interface
public:
    virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload) override;
    virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload) override;
    virtual FString GetNotifyName_Implementation() const override;
#if WITH_EDITOR
    virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override;
#endif
    //~ End UAnimNotifyState Interface
};
