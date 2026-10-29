// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "P1ShopRow.generated.h"

/** Shop 표(Shop.xlsx)의 한 행. 행 이름은 키(Id)다. */
USTRUCT(BlueprintType)
struct FP1ShopRow : public FTableRowBase
{
    GENERATED_BODY()

    /** 상점 번호. NPC 표가 가리킨다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Id = 0;

    /** 파는 아이템. ';'로 구분하고 상점 창의 진열 순서다. 가격은 아이템 표의 BuyPrice다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<int32> Items;
};
