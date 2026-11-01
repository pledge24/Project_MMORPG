// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Game/Data/Generated/P1ActionHitRow.h"
#include "P1ActionRow.generated.h"

/** Action 표(Action.xlsx)의 한 행. 행 이름은 키(Id)다. */
USTRUCT(BlueprintType)
struct FP1ActionRow : public FTableRowBase
{
    GENERATED_BODY()

    /** 액션 번호. 직업의 액션은 1000번대, 몬스터의 액션은 2000번대다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Id = 0;

    /** 액션 이름 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Name;

    /** ActionHit 표에서 이 행에 속한 행. 키 순서다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FP1ActionHitRow> Hits;
};
