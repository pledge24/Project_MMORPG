#pragma once

#include "CoreMinimal.h"
#include "Game/World/P1PlacementMarker.h"
#include "P1MonsterSpawnMarker.generated.h"

class USphereComponent;

/**
 * 몬스터가 나오는 지점.
 *
 * 스폰 그룹 표의 그룹 하나를 가리키고, 그 그룹의 몬스터가 나와 배회하는 반경을 정한다.
 * 액터의 위치는 걸을 수 있는 바닥 위에 둔다.
 */
UCLASS()
class P1_API AP1MonsterSpawnMarker : public AP1PlacementMarker
{
    GENERATED_BODY()

public:
    AP1MonsterSpawnMarker();

    //~ Begin AActor Interface
protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    //~ End AActor Interface

    //~ 스폰
public:
    int32 GetSpawnGroupId() const { return SpawnGroupId; }
    float GetRadius() const { return Radius; }

private:
    /** 스폰 그룹 표의 Id. */
    UPROPERTY(EditAnywhere, Category = "Placement")
    int32 SpawnGroupId = 0;

    /** 몬스터가 나오고 배회하는 평면 반경(cm). 액터의 크기는 보지 않는다. 몬스터의 추적 한계(ChasingMaxRange)보다 작아야 한다. */
    UPROPERTY(EditAnywhere, Category = "Placement", meta = (ClampMin = "1.0"))
    float Radius = 500.f;

    /** 에디터에서 반경을 보여 준다. 충돌하지 않는다. */
    UPROPERTY(VisibleAnywhere, Category = "Placement")
    TObjectPtr<USphereComponent> RadiusPreview;
};
