#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1PlacementMarker.generated.h"

class UArrowComponent;

/**
 * 서버가 알아야 하는 공간 정보를 레벨에 표시하는 마커의 베이스.
 *
 * 마커는 에디터 전용 액터라 쿠킹한 빌드에는 없다. 배치 추출 커맨드렛이 마커를 읽어 서버의 배치 데이터를 만든다.
 * 마커에는 공간과 기획 데이터의 행 번호만 두고, 규칙과 수치는 기획 데이터 표에 둔다.
 */
UCLASS(Abstract)
class P1_API AP1PlacementMarker : public AActor
{
    GENERATED_BODY()

public:
    AP1PlacementMarker();

private:
    /** 마커가 바라보는 방향. 리스폰 지점과 포털 도착 지점은 이 방향으로 선다. */
    UPROPERTY(VisibleAnywhere, Category = "Placement")
    TObjectPtr<UArrowComponent> Direction;
};
