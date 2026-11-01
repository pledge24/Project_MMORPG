#include "Game/Combat/P1AnimNotify_AttackHit.h"
#include "Components/SkeletalMeshComponent.h"
#include "Game/Combat/P1ActionComponent.h"

UP1AnimNotify_AttackHit::UP1AnimNotify_AttackHit()
{
#if WITH_EDITORONLY_DATA
    // 몽타주 편집기에서 다른 노티파이와 구별되게 칠한다.
    NotifyColor = FColor(220, 40, 40);
#endif
}

void UP1AnimNotify_AttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    // 몽타주 편집기의 미리보기 메시에는 액션 컴포넌트가 없다.
    const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
    if (UP1ActionComponent* Action = Owner ? Owner->FindComponentByClass<UP1ActionComponent>() : nullptr)
        Action->HandleAttackHitNotify();
}

FString UP1AnimNotify_AttackHit::GetNotifyName_Implementation() const
{
    return TEXT("AttackHit");
}
