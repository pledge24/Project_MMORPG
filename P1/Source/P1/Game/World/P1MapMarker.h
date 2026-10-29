#pragma once

#include "CoreMinimal.h"
#include "Game/World/P1PlacementVolume.h"
#include "P1MapMarker.generated.h"

/**
 * 맵의 경계와 AOI 셀 크기를 정하는 마커.
 *
 * 맵 레벨마다 하나 둔다. 지역 볼륨과 다른 마커는 모두 이 경계 안에 있어야 한다.
 */
UCLASS()
class P1_API AP1MapMarker : public AP1PlacementVolume
{
    GENERATED_BODY()

public:
    float GetCellSize() const { return CellSize; }

private:
    /** 서버가 맵을 AOI 셀로 나눌 때 셀 한 변의 길이(cm). 배치 데이터에 실린다. */
    UPROPERTY(EditAnywhere, Category = "Placement", meta = (ClampMin = "1"))
    float CellSize = 1000.f;
};
