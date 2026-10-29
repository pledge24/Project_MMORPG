#include "Game/Entities/P1Npc.h"
#include "Animation/AnimationAsset.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/P1NpcAssetData.h"
#include "Materials/MaterialInterface.h"

AP1Npc::AP1Npc()
{
    PrimaryActorTick.bCanEverTick = false;

    // 플레이어는 NPC를 지나갈 수 없고, 조준 스윕(Visibility)은 NPC에서 멈춘다. 카메라 붐은 NPC에 당겨지지 않는다.
    Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
    Capsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
    Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    RootComponent = Capsule;

    Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Capsule);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AP1Npc::Initialize(int64 InEntityId, int32 InNpcId)
{
    EntityId = InEntityId;
    NpcId = InNpcId;
}

void AP1Npc::ApplyAssetData(const FP1NpcAssetData& AssetData)
{
    Capsule->SetCapsuleSize(AssetData.CapsuleRadius, AssetData.CapsuleHalfHeight);

    Mesh->SetSkeletalMeshAsset(AssetData.SkeletalMesh.LoadSynchronous());
    Mesh->SetRelativeLocationAndRotation(AssetData.MeshOffset, AssetData.MeshRotation);

    // 애니메이션 블루프린트 없이 대기 애니메이션 하나를 반복한다.
    if (UAnimationAsset* Idle = AssetData.IdleAnimation.LoadSynchronous())
    {
        Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Mesh->PlayAnimation(Idle, true);
    }
}

float AP1Npc::GetCapsuleHalfHeight() const
{
    return Capsule->GetScaledCapsuleHalfHeight();
}

void AP1Npc::SetFocused(bool bFocused)
{
    UMaterialInterface* Overlay = bFocused ? GetDefault<UP1GameDataSettings>()->NpcFocusOverlayMaterial.LoadSynchronous() : nullptr;
    Mesh->SetOverlayMaterial(Overlay);
}

FVector AP1Npc::GetCameraFocusPoint() const
{
    // 캡슐 중심에서 반높이의 0.7만큼 올라간 곳. 사람 모양 메시의 얼굴 근처다.
    return GetActorLocation() + FVector(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight() * 0.7f);
}
