#pragma once

#include "CoreMinimal.h"
#include "Game/World/P1PlacementMarker.h"
#include "P1PortalArrivalMarker.generated.h"

/**
 * 포털을 탄 플레이어가 서는 지점.
 *
 * 포털 표의 도착 지역 안에 둔다. 다른 맵으로 가는 포털이면 도착 맵의 레벨에 둔다.
 */
UCLASS()
class P1_API AP1PortalArrivalMarker : public AP1PlacementMarker
{
    GENERATED_BODY()

public:
    int32 GetPortalId() const { return PortalId; }

private:
    /** 포털 표의 Id. 출발하는 포털 액터의 포털 번호와 같다. */
    UPROPERTY(EditAnywhere, Category = "Placement")
    int32 PortalId = 0;
};
