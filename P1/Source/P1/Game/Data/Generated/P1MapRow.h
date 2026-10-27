// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "P1MapRow.generated.h"

/** Map 표(Map.xlsx)의 한 행. 행 이름은 키(Id)다. */
USTRUCT(BlueprintType)
struct FP1MapRow : public FTableRowBase
{
    GENERATED_BODY()

    /** 맵 번호. 클라이언트가 이 번호로 열 레벨을 고른다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Id = 0;

    /** 맵 이름 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Name;
};
