#pragma once

#include "CoreMinimal.h"
#include "Game/World/P1PlacementMarker.h"
#include "P1NpcMarker.generated.h"

/**
 * 서버가 NPC를 세우는 지점.
 *
 * NPC 표의 NPC마다 어느 맵 레벨에 하나 둔다. 마커를 바닥에 두면 NPC가 그 바닥에 서고 화살표 방향을 본다.
 */
UCLASS()
class P1_API AP1NpcMarker : public AP1PlacementMarker
{
    GENERATED_BODY()

public:
    int32 GetNpcId() const { return NpcId; }

private:
    /** NPC 표의 Id. */
    UPROPERTY(EditAnywhere, Category = "Placement")
    int32 NpcId = 0;
};
