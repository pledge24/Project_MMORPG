#pragma once

#include "CoreMinimal.h"
#include "Game/World/P1PlacementMarker.h"
#include "P1PlacementVolume.generated.h"

class UBoxComponent;

/**
 * 바닥에 놓는 상자 모양의 배치 마커.
 *
 * 액터의 위치가 상자 바닥의 중심이다. 서버는 상자를 평면의 축 정렬 경계로 쓰므로 회전하지 않는다.
 */
UCLASS(Abstract)
class P1_API AP1PlacementVolume : public AP1PlacementMarker
{
    GENERATED_BODY()

public:
    AP1PlacementVolume();

    //~ Begin AActor Interface
protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    //~ End AActor Interface

    //~ Bounds
public:
    /** 액터의 크기를 포함한 X, Y 방향 반경. */
    FVector2D GetHalfExtent() const;

private:
    /** 상자의 크기는 Box Extent로 정한다. 높이는 서버가 보지 않는다. */
    UPROPERTY(VisibleAnywhere, Category = "Placement")
    TObjectPtr<UBoxComponent> Bounds;
};
