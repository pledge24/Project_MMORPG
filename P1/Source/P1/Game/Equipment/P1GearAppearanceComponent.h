#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "P1GearAppearanceComponent.generated.h"

class UDataTable;
class UMeshComponent;

/**
 * 장비 부위마다 메시 컴포넌트를 하나씩 맡아, 그 부위에 장착한 아이템의 메시로 바꾼다.
 * 메시 컴포넌트는 소유 액터가 만들어 RegisterGearMesh로 넘긴다.
 */
UCLASS(ClassGroup = (P1), meta = (BlueprintSpawnableComponent))
class P1_API UP1GearAppearanceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UP1GearAppearanceComponent();

    //~ Gear Appearance
public:
    /** 장비 부위(GearType)의 외형을 맡을 메시 컴포넌트를 등록한다. 스켈레탈 메시와 스태틱 메시만 받는다. */
    void RegisterGearMesh(int32 GearType, UMeshComponent* MeshComponent);

    /** 장비 부위에 아이템 외형을 입힌다. TemplateId가 0이면 그 부위를 비운다. */
    void ApplyGear(int32 GearType, int32 TemplateId);

private:
    /** 아이템 메시를 담은 에셋 테이블이다. 행 구조체는 FP1ItemAssetData다. */
    UPROPERTY(EditDefaultsOnly, Category = "Data")
    TObjectPtr<UDataTable> ItemAssetTable;

    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UMeshComponent>> GearMeshes;
};
