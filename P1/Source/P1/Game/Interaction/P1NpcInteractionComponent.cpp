#include "Game/Interaction/P1NpcInteractionComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/Generated/P1InteractionConfig.h"
#include "Game/Data/Generated/P1NpcRow.h"
#include "Game/Entities/P1MyPlayer.h"
#include "Game/Entities/P1Npc.h"
#include "Game/Interaction/P1NpcFocus.h"
#include "Game/Interaction/P1ShopScreen.h"
#include "Utils/LogCategory.h"

UP1NpcInteractionComponent::UP1NpcInteractionComponent()
{
    // 카메라와 캐릭터가 이번 프레임에 움직인 뒤에 조준한다.
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UP1NpcInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 상점을 연 NPC가 사라졌으면(룸 이동, 시야 밖) 상점을 닫는다. 그 NPC와는 더 거래할 수 없다.
    if (ShopNpc.IsStale())
        CloseShop();

    // 상점이 열린 동안에는 다른 NPC를 조준하지 않는다.
    if (IsShopOpen())
        SetFocus(nullptr);
    else
        UpdateFocus();

    UpdateCamera(DeltaTime);
}

void UP1NpcInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SetFocus(nullptr);
    ShopNpc.Reset();

    Super::EndPlay(EndPlayReason);
}

void UP1NpcInteractionComponent::Interact()
{
    AP1Npc* Npc = FocusedNpc.Get();
    IP1ShopScreen* Screen = GetScreen();
    AP1MyPlayer* MyPlayer = GetMyPlayer();
    if (Npc == nullptr || Screen == nullptr || MyPlayer == nullptr || IsShopOpen())
        return;

    const FP1NpcRow* NpcRow = UP1GameDataSettings::FindNpcRow(Npc->GetNpcId());
    if (NpcRow == nullptr)
    {
        UE_LOG(LogP1CharacterInteraction, Warning, TEXT("NPC 표에 %d번 NPC가 없어 상점을 열지 않는다"), Npc->GetNpcId());
        return;
    }

    // 닫으면 돌아올 카메라를 저장하고 NPC 쪽으로 당긴다. 앞 상점에서 돌아오는 중이면 그때 저장한 카메라가 원래 카메라다.
    if (bRestoringCamera == false)
    {
        SavedArmLength = MyPlayer->GetCameraBoom()->TargetArmLength;
        SavedControlRotation = GetPlayerController()->GetControlRotation();
    }
    bRestoringCamera = false;

    ShopNpc = Npc;
    SetFocus(nullptr);
    Screen->OpenShopWindow(Npc->GetEntityId(), NpcRow->ShopId);
}

void UP1NpcInteractionComponent::UpdateFocus()
{
    APlayerController* PlayerController = GetPlayerController();
    AP1MyPlayer* MyPlayer = GetMyPlayer();
    UWorld* World = GetWorld();
    if (PlayerController == nullptr || PlayerController->PlayerCameraManager == nullptr || MyPlayer == nullptr || World == nullptr)
    {
        SetFocus(nullptr);
        return;
    }

    // 카메라가 캐릭터 뒤에 있으므로 스윕은 캐릭터까지의 거리에 사거리의 두 배를 더한 만큼 간다.
    const float Range = P1DesignData::Interaction::InteractionRange;
    const FVector Start = PlayerController->PlayerCameraManager->GetCameraLocation();
    const FVector Direction = PlayerController->PlayerCameraManager->GetCameraRotation().Vector();
    const FVector End = Start + Direction * (FVector::Dist(Start, MyPlayer->GetActorLocation()) + Range * 2.f);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(P1NpcAim), false, MyPlayer);
    TArray<FHitResult> SweepHits;
    World->SweepMultiByChannel(SweepHits, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(AIM_SWEEP_RADIUS), Params);

    TArray<FP1AimHit> Hits;
    TArray<AActor*> HitActors;
    for (const FHitResult& SweepHit : SweepHits)
    {
        // 카메라가 이미 들어가 있던 것은 조준한 것이 아니다.
        AActor* HitActor = SweepHit.GetActor();
        if (SweepHit.bStartPenetrating || HitActor == nullptr)
            continue;

        Hits.Add({ static_cast<float>(SweepHit.Distance), HitActor->GetActorLocation(), HitActor->IsA<AP1Npc>() });
        HitActors.Add(HitActor);
    }

    const int32 Picked = P1NpcFocus::PickFocus(Hits, MyPlayer->GetActorLocation(), Range, MyPlayer->bBattleMode);
    SetFocus(Picked != INDEX_NONE ? Cast<AP1Npc>(HitActors[Picked]) : nullptr);
}

void UP1NpcInteractionComponent::SetFocus(AP1Npc* Npc)
{
    AP1Npc* Previous = FocusedNpc.Get();
    if (Previous == Npc && FocusedNpc.IsStale() == false)
        return;

    if (Previous != nullptr)
        Previous->SetFocused(false);

    FocusedNpc = Npc;
    if (Npc != nullptr)
        Npc->SetFocused(true);

    // 지금 NPC는 모두 상점을 연다. 다른 상호작용이 생기면 NPC 표에서 문구를 고른다.
    if (IP1ShopScreen* Screen = GetScreen())
        Screen->ShowInteractionPrompt(Npc != nullptr ? FText::FromString(TEXT("상점 열기")) : FText::GetEmpty());
}

void UP1NpcInteractionComponent::CloseShop()
{
    if (ShopNpc.IsExplicitlyNull())
        return;

    ShopNpc.Reset();
    bRestoringCamera = true;

    if (IP1ShopScreen* Screen = GetScreen())
        Screen->CloseShopWindow();
}

void UP1NpcInteractionComponent::UpdateCamera(float DeltaTime)
{
    APlayerController* PlayerController = GetPlayerController();
    AP1MyPlayer* MyPlayer = GetMyPlayer();
    if (PlayerController == nullptr || MyPlayer == nullptr)
        return;

    USpringArmComponent* Boom = MyPlayer->GetCameraBoom();
    const FRotator Current = PlayerController->GetControlRotation();
    if (AP1Npc* Npc = ShopNpc.Get())
    {
        // 카메라 회전 입력은 막혀 있다. 컨트롤러 회전을 직접 돌려 붐의 축이 NPC의 얼굴을 보게 한다.
        const FRotator Target = (Npc->GetCameraFocusPoint() - Boom->GetComponentLocation()).Rotation();
        PlayerController->SetControlRotation(FMath::RInterpTo(Current, Target, DeltaTime, CAMERA_INTERP_SPEED));
        Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength, SHOP_ARM_LENGTH, DeltaTime, CAMERA_INTERP_SPEED);
        return;
    }

    if (bRestoringCamera == false)
        return;

    const FRotator Restored = FMath::RInterpTo(Current, SavedControlRotation, DeltaTime, CAMERA_INTERP_SPEED);
    PlayerController->SetControlRotation(Restored);
    Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength, SavedArmLength, DeltaTime, CAMERA_INTERP_SPEED);

    // 거의 다 돌아왔으면 맞춰 두고 끝낸다. 보간만으로는 끝에 닿지 않는다.
    if (Restored.Equals(SavedControlRotation, 0.5f) && FMath::IsNearlyEqual(Boom->TargetArmLength, SavedArmLength, 1.f))
    {
        PlayerController->SetControlRotation(SavedControlRotation);
        Boom->TargetArmLength = SavedArmLength;
        bRestoringCamera = false;
    }
}

APlayerController* UP1NpcInteractionComponent::GetPlayerController() const
{
    return Cast<APlayerController>(GetOwner());
}

AP1MyPlayer* UP1NpcInteractionComponent::GetMyPlayer() const
{
    const APlayerController* PlayerController = GetPlayerController();
    return PlayerController ? Cast<AP1MyPlayer>(PlayerController->GetPawn()) : nullptr;
}

IP1ShopScreen* UP1NpcInteractionComponent::GetScreen() const
{
    return Cast<IP1ShopScreen>(GetOwner());
}
