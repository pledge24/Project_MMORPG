#include "Game/Combat/P1ActionComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Combat.pb.h"
#include "Game/Combat/P1AttackAreaDebugDraw.h"
#include "Game/Data/P1ActionAssetData.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/Generated/P1ActionRow.h"
#include "Network/P1PacketSender.h"
#include "Utils/LogCategory.h"

namespace
{
    /** 콤보 입력 창 노티파이 스테이트가 몽타주 노티파이 이벤트로 보내는 이름. */
    const FName COMBO_WINDOW_NOTIFY_NAME(TEXT("ComboWindow"));

    int32 FindNextComboAction(int32 ActionId)
    {
        const FP1ActionRow* Row = UP1GameDataSettings::FindActionRow(ActionId);
        return Row ? Row->NextAction : 0;
    }
}

UP1ActionComponent::UP1ActionComponent()
    : Combo(&FindNextComboAction)
{
}

void UP1ActionComponent::InputNormalAttack(int32 FirstActionId)
{
    if (CanInputAction() == false)
        return;

    // 진행 중이면 다음 단계를 예약할 뿐이다. 예약한 단계는 입력 창이 닫힐 때 시작한다.
    const int32 StartedAction = Combo.Input(FirstActionId);
    if (StartedAction != 0 && StartLocalAction(StartedAction) == false)
        Combo.EndAction();
}

void UP1ActionComponent::CancelLocalAction()
{
    const int32 CanceledAction = PlayingActionId;

    // 상태를 먼저 비운다. 멈춘 몽타주의 끝 이벤트는 블렌드 아웃 뒤에 오고, 그사이 새로 누른 공격은 첫 단계부터 시작해야 한다.
    PlayingActionId = 0;
    ActionMontageInstanceId = INDEX_NONE;
    Combo.EndAction();

    const FP1ActionAssetData* AssetData = CanceledAction != 0 ? UP1GameDataSettings::FindActionAssetData(CanceledAction) : nullptr;
    UAnimMontage* Montage = AssetData ? AssetData->Montage.Get() : nullptr;
    UAnimInstance* AnimInstance = GetAnimInstance();
    if (Montage && AnimInstance && AnimInstance->Montage_IsPlaying(Montage))
        AnimInstance->Montage_Stop(Montage->GetDefaultBlendOutTime(), Montage);
}

bool UP1ActionComponent::StartLocalAction(int32 ActionId)
{
    if (PlayActionSection(ActionId) == false)
        return false;

    // 클라이언트가 몽타주를 먼저 재생하고 알린다. 서버의 위치와 방향은 마지막 이동 패킷의 값이라 늦을 수 있다.
    const AActor* Owner = GetOwner();
    const FVector Location = Owner->GetActorLocation();
    Protocol::C_ACTION ActionPkt;
    {
        ActionPkt.set_action_id(ActionId);
        ActionPkt.set_yaw(Owner->GetActorRotation().Yaw);
        ActionPkt.mutable_start_pos()->set_x(Location.X);
        ActionPkt.mutable_start_pos()->set_y(Location.Y);
        ActionPkt.mutable_start_pos()->set_z(Location.Z);
    }
    FP1PacketSender::Send(this, ActionPkt);
    return true;
}

void UP1ActionComponent::HandleMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
    if (NotifyName != COMBO_WINDOW_NOTIFY_NAME || Payload.MontageInstanceID != ActionMontageInstanceId)
        return;

    Combo.BeginWindow();
}

void UP1ActionComponent::HandleMontageNotifyEnd(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
    if (NotifyName != COMBO_WINDOW_NOTIFY_NAME || Payload.MontageInstanceID != ActionMontageInstanceId)
        return;

    // 창 안에서 누른 입력이 있으면 창이 닫히는 지금 다음 단계로 넘어간다. 섹션 끝까지 기다리면 회복 동작을 다 본 뒤에야 이어진다.
    // 같은 몽타주 안의 섹션 이동에는 블렌드가 없다.
    const int32 NextAction = Combo.EndWindow();
    if (NextAction != 0 && StartLocalAction(NextAction) == false)
        Combo.EndAction();
}

void UP1ActionComponent::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 블렌드 아웃 중에 다음 단계가 같은 몽타주를 다시 재생하면 앞 재생의 끝이 뒤늦게 온다. 새 재생이 상태를 쥐고 있으므로 건드리지 않는다.
    const UAnimInstance* AnimInstance = GetAnimInstance();
    if (AnimInstance && AnimInstance->Montage_IsPlaying(Montage))
        return;

    // 섹션끼리 잇지 않았으므로 다음 단계 없이 섹션이 끝나면 몽타주가 끝난다. 다른 몽타주가 끊어도 액션은 끝난다.
    // 콤보가 끝나고 다음 입력은 첫 단계부터다.
    PlayingActionId = 0;
    ActionMontageInstanceId = INDEX_NONE;
    Combo.EndAction();
}

void UP1ActionComponent::PlayRemoteAction(int32 ActionId)
{
    // 내 플레이어는 입력에서 이미 재생했다. 로컬 플레이어 컨트롤러가 조종하는 폰은 내 플레이어뿐이다.
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn && OwnerPawn->IsPlayerControlled() && OwnerPawn->IsLocallyControlled())
        return;

    PlayActionSection(ActionId);
}

void UP1ActionComponent::HandleAttackHitNotify()
{
    if (PlayingActionId == 0)
        return;

    // 서버의 위치는 캡슐 중심이고 공격 범위의 높이는 발밑 기준이다.
    const ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (Character == nullptr)
        return;

    const float HalfHeight = Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
    const FVector Foot = Character->GetActorLocation() - FVector(0.f, 0.f, HalfHeight);
    P1AttackAreaDebugDraw::DrawHitIfEnabled(GetWorld(), PlayingActionId, NextHitIndex++, Foot, Character->GetActorRotation().Yaw);
}

void UP1ActionComponent::InRestrictedArea()
{
    RestrictedArea++;
}

void UP1ActionComponent::OutRestrictedArea()
{
    RestrictedArea = FMath::Max(RestrictedArea - 1, 0);
}

bool UP1ActionComponent::PlayActionSection(int32 ActionId)
{
    const FP1ActionAssetData* AssetData = UP1GameDataSettings::FindActionAssetData(ActionId);
    UAnimMontage* Montage = AssetData ? AssetData->Montage.LoadSynchronous() : nullptr;
    UAnimInstance* AnimInstance = GetAnimInstance();
    if (Montage == nullptr || AnimInstance == nullptr)
    {
        UE_LOG(LogP1Combat, Warning, TEXT("%s: 액션 %d의 몽타주나 애님 인스턴스가 없어 재생하지 않는다"), *GetNameSafe(GetOwner()), ActionId);
        return false;
    }

    // 콤보의 다음 단계는 같은 몽타주의 다음 섹션이다. 다시 재생하면 몽타주가 처음부터 블렌드 인하고 끝 이벤트가 중단으로 온다.
    if (AnimInstance->Montage_IsPlaying(Montage) == false)
    {
        if (AnimInstance->Montage_Play(Montage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, true) <= 0.f)
            return false;

        // 이 재생의 끝과 노티파이만 받는다. 다른 몽타주가 끊은 앞 재생의 이벤트는 섞이지 않는다.
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &UP1ActionComponent::HandleMontageEnded);
        AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
        AnimInstance->OnPlayMontageNotifyBegin.AddUniqueDynamic(this, &UP1ActionComponent::HandleMontageNotifyBegin);
        AnimInstance->OnPlayMontageNotifyEnd.AddUniqueDynamic(this, &UP1ActionComponent::HandleMontageNotifyEnd);
    }
    AnimInstance->Montage_JumpToSection(AssetData->Section, Montage);

    const FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(Montage);
    ActionMontageInstanceId = MontageInstance ? MontageInstance->GetInstanceID() : INDEX_NONE;
    PlayingActionId = ActionId;
    NextHitIndex = 0;
    return true;
}

UAnimInstance* UP1ActionComponent::GetAnimInstance() const
{
    // 몬스터는 스폰한 뒤 표현 데이터로 애님 클래스를 바꾸므로 재생할 때마다 찾는다.
    const ACharacter* Character = Cast<ACharacter>(GetOwner());
    return Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
}
