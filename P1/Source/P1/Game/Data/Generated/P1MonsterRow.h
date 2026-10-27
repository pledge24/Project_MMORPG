// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Game/Data/Generated/P1MonsterAttackType.h"
#include "P1MonsterRow.generated.h"

/** Monster 표(Monster.xlsx)의 한 행. 행 이름은 키(Id)다. */
USTRUCT(BlueprintType)
struct FP1MonsterRow : public FTableRowBase
{
    GENERATED_BODY()

    /** 몬스터 번호. 5000번대다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Id = 0;

    /** 몬스터 이름 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Name;

    /** 몬스터 레벨 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Level = 0;

    /** 공격 방식. 근거리(MELEE)나 원거리(RANGED) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EP1MonsterAttackType AttackType = EP1MonsterAttackType::None;

    /** 최대 HP */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 MaxHp = 0;

    /** 공격력 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 BaseAttack = 0;

    /** 이동 속도 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float MovementSpeed = 0.f;
};
