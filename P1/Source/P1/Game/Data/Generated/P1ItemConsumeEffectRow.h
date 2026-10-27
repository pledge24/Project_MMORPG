// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "P1ItemConsumeEffectRow.generated.h"

/** ItemConsumeEffect 표(ItemConsumeEffect.xlsx)의 한 행. 행 이름은 키(ItemId)다. */
USTRUCT(BlueprintType)
struct FP1ItemConsumeEffectRow : public FTableRowBase
{
    GENERATED_BODY()

    /** 소모품 아이템 번호 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 ItemId = 0;

    /** 회복하는 HP의 비율. 최대 HP 기준이다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float HpRestoreRatio = 0.f;

    /** 회복하는 MP의 비율. 최대 MP 기준이다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float MpRestoreRatio = 0.f;

    /** 같은 아이템을 다시 쓰기까지의 대기(ms). 0이면 대기가 없다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 CooldownMs = 0;
};
