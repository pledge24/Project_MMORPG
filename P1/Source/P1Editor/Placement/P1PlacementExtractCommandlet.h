#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "P1PlacementExtractCommandlet.generated.h"

/**
 * 게임 맵의 레벨에서 배치 마커를 읽어 서버의 배치 데이터를 쓰는 커맨드렛.
 *
 * 프로젝트 설정의 맵 표(MapLevels)에 있는 레벨을 모두 열고, 마커와 기획 데이터를 대조해 문제가 없을 때만 산출물을 쓴다.
 * 문제가 있으면 오류를 한 줄씩 남기고 1로 끝난다. 실행은 P1/Scripts/Invoke-PlacementExtract.ps1이 맡는다.
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
