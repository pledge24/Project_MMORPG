#pragma once

#include "CoreMinimal.h"
#include "Game/World/P1PlacementMarker.h"
#include "P1RespawnPointMarker.generated.h"

/**
 * 사망한 플레이어가 마을 리스폰으로 서는 지점.
 *
 * 리스폰 지점이 있는 지역(마을)의 지역 볼륨 안에 하나 둔다. 소속 지역은 추출이 위치로 정한다.
 */
UCLASS()
class P1_API AP1RespawnPointMarker : public AP1PlacementMarker
{
    GENERATED_BODY()
};
