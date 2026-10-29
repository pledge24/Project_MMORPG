#pragma once

#include "CoreMinimal.h"
#include "Game/World/P1PlacementVolume.h"
#include "P1AreaVolume.generated.h"

/**
 * 지역 표의 지역 하나가 차지하는 공간.
 *
 * 지역의 이름, 유형, 몬스터 같은 규칙은 지역 표에 있고, 이 볼륨은 경계와 지역 번호만 갖는다.
 * 경계 벽이 이 볼륨의 크기로 맞춰진다.
 */
UCLASS()
class P1_API AP1AreaVolume : public AP1PlacementVolume
{
    GENERATED_BODY()

    //~ Begin AActor Interface
#if WITH_EDITOR
public:
    virtual void PostEditMove(bool bFinished) override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
    //~ End AActor Interface

    //~ Area
public:
    int32 GetAreaId() const { return AreaId; }

private:
    /** 이 볼륨을 가리키는 경계 벽을 다시 맞춘다. */
    void RefitBoundaryWalls();

    /** 지역 표의 Id. */
    UPROPERTY(EditAnywhere, Category = "Placement")
    int32 AreaId = 0;
};
