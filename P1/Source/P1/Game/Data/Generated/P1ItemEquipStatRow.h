// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Game/Data/Generated/P1DesignDataEnumNames.h"
#include "Network/Common.pb.h"
#include "P1ItemEquipStatRow.generated.h"

/** ItemEquipStat 표(ItemEquipStat.xlsx)의 한 행. 행 이름은 키(ItemId)다. */
USTRUCT(BlueprintType)
struct FP1ItemEquipStatRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Part 값을 프로토콜 열거형으로 읽는다. 값 이름이 아니면 0번 값이다. */
    Protocol::GearPart GetPart() const
    {
        Protocol::GearPart Parsed{};
        P1DesignData::ParseEnumName(Part, Parsed);
        return Parsed;
    }

    /** 장비 아이템 번호 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 ItemId = 0;

    /** 착용 부위 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Part;

    /** 착용하면 오르는 최대 HP */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Hp = 0;

    /** 착용하면 오르는 최대 MP */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Mp = 0;

    /** 착용하면 오르는 물리 공격력 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 PhysicalAttack = 0;

    /** 착용하면 오르는 마법 공격력 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 MagicalAttack = 0;
};
