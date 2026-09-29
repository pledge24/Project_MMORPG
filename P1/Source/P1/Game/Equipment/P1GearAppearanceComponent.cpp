#include "Game/Equipment/P1GearAppearanceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Game/Data/P1ItemAssetData.h"
#include "Utils/LogCategory.h"

UP1GearAppearanceComponent::UP1GearAppearanceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UP1GearAppearanceComponent::RegisterGearMesh(int32 GearType, UMeshComponent* MeshComponent)
{
    if (MeshComponent == nullptr)
        return;

    GearMeshes.Add(GearType, MeshComponent);
}

void UP1GearAppearanceComponent::ApplyGear(int32 GearType, int32 TemplateId)
{
    TObjectPtr<UMeshComponent>* FoundMesh = GearMeshes.Find(GearType);
    if (FoundMesh == nullptr || *FoundMesh == nullptr)
    {
        UE_LOG(LogP1Entity, Warning, TEXT("%s: 장비 부위 %d에 등록된 메시 컴포넌트가 없음"), *GetNameSafe(GetOwner()), GearType);
        return;
    }

    // 행을 못 찾으면 이전 아이템 메시가 남지 않도록 부위를 비운다.
    const FP1ItemAssetData* AssetData = nullptr;
    if (TemplateId > 0)
    {
        if (ItemAssetTable == nullptr)
        {
            UE_LOG(LogP1Entity, Warning, TEXT("%s: ItemAssetTable이 지정되지 않음"), *GetNameSafe(GetOwner()));
        }
        else
        {
            AssetData = ItemAssetTable->FindRow<FP1ItemAssetData>(FName(*FString::FromInt(TemplateId)), TEXT("UP1GearAppearanceComponent::ApplyGear"));
        }

        if (AssetData == nullptr)
        {
            UE_LOG(LogP1Entity, Warning, TEXT("%s: 아이템 %d의 에셋 행이 없어 부위 %d를 비움"), *GetNameSafe(GetOwner()), TemplateId, GearType);
        }
    }

    // 장착 한 번에 한 개체만 부르므로 지금은 동기 로드로 충분하다.
    if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(*FoundMesh))
    {
        SkeletalMeshComponent->SetSkeletalMeshAsset(AssetData ? AssetData->SkeletalMesh.LoadSynchronous() : nullptr);
    }
    else if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(*FoundMesh))
    {
        StaticMeshComponent->SetStaticMesh(AssetData ? AssetData->StaticMesh.LoadSynchronous() : nullptr);
    }
}
