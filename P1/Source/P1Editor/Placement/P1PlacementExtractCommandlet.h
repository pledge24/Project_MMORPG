#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "P1PlacementExtractCommandlet.generated.h"

/**
 * 게임 맵의 레벨에서 배치 마커와 충돌 형상을 읽어 서버의 배치 데이터와 navmesh 빌드의 입력을 쓰는 커맨드렛.
 *
 * 레벨에 놓인 공간 정보를 서버가 읽는 형식으로 옮기고, 마커와 기획 데이터가 맞지 않으면 산출물을 바꾸지 않는다.
 */
UCLASS()
class UP1PlacementExtractCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UP1PlacementExtractCommandlet();

    //~ Begin UCommandlet Interface
public:
    virtual int32 Main(const FString& Params) override;
    //~ End UCommandlet Interface
};
