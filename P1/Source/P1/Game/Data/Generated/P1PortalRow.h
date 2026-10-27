// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "P1PortalRow.generated.h"

/** Portal 표(Portal.xlsx)의 한 행. Area 행의 Portals 아래에 키 순서로 실린다. */
USTRUCT(BlueprintType)
struct FP1PortalRow
{
    GENERATED_BODY()

    /** 포털 번호. 클라이언트가 포털 이동을 요청할 때 보낸다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Id = 0;
};
