// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Game/Data/Generated/P1AreaType.h"
#include "Game/Data/Generated/P1PortalRow.h"
#include "P1AreaRow.generated.h"

/** Area 표(Area.xlsx)의 한 행. 행 이름은 키(Id)다. */
USTRUCT(BlueprintType)
struct FP1AreaRow : public FTableRowBase
{
    GENERATED_BODY()

    /** 지역 번호 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Id = 0;

    /** 이 지역이 속한 맵 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 MapId = 0;

    /** 지역 이름 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Name;

    /** 지역 유형. 안전 지대(SAFE_ZONE)나 사냥터(HUNTING_FIELD) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EP1AreaType Type = EP1AreaType::None;

    /** 리스폰 지점이 있으면 TRUE. 리스폰 지점이 있는 지역(마을)은 하나뿐이다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool HasRespawnPoint = false;

    /** Portal 표에서 이 행에 속한 행. 키 순서다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FP1PortalRow> Portals;
};
