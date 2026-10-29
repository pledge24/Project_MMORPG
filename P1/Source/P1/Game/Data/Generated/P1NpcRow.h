// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "P1NpcRow.generated.h"

/** Npc 표(Npc.xlsx)의 한 행. 행 이름은 키(Id)다. */
USTRUCT(BlueprintType)
struct FP1NpcRow : public FTableRowBase
{
    GENERATED_BODY()

    /** NPC 번호. 레벨의 NPC 마커가 이 번호를 가리킨다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Id = 0;

    /** NPC 이름 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Name;

    /** 조준해 클릭하면 여는 상점 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 ShopId = 0;
};
