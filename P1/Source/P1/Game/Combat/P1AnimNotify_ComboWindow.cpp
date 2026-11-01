#include "Game/Combat/P1AnimNotify_ComboWindow.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
    const FName COMBO_WINDOW_NOTIFY_NAME(TEXT("ComboWindow"));
}

UP1AnimNotify_ComboWindow::UP1AnimNotify_ComboWindow()
{
    // 분기점 노티파이여야 몽타주 인스턴스 번호와 함께 프레임에 맞춰 불린다. 콤보 처리는 그 번호로 다른 몽타주의 노티파이를 거른다.
    bIsNativeBranchingPoint = true;

#if WITH_EDITORONLY_DATA
    // 몽타주 편집기에서 타격 노티파이(빨강)와 구별되게 칠한다.
    NotifyColor = FColor(40, 160, 220);
#endif
}

void UP1AnimNotify_ComboWindow::BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload)
{
    Super::BranchingPointNotify(BranchingPointPayload);

    // 범용 몽타주 노티파이와 같은 이벤트로 알린다. 지금 콤보 처리는 이 이벤트의 어떤 노티파이든 다음 입력을 연다.
    if (USkeletalMeshComponent* MeshComp = BranchingPointPayload.SkelMeshComponent)
    {
        if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
            AnimInstance->OnPlayMontageNotifyBegin.Broadcast(COMBO_WINDOW_NOTIFY_NAME, BranchingPointPayload);
    }
}

FString UP1AnimNotify_ComboWindow::GetNotifyName_Implementation() const
{
    return COMBO_WINDOW_NOTIFY_NAME.ToString();
}

#if WITH_EDITOR
bool UP1AnimNotify_ComboWindow::CanBePlaced(UAnimSequenceBase* Animation) const
{
    // 몽타주 노티파이 이벤트는 몽타주에서만 불린다.
    return Animation && Animation->IsA<UAnimMontage>();
}
#endif
