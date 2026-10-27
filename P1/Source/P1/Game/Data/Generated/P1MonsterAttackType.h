// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "P1MonsterAttackType.generated.h"

/** 기획 데이터 전용 열거형 MonsterAttackType. 값 번호가 서버의 생성된 열거형과 같다. */
UENUM(BlueprintType)
enum class EP1MonsterAttackType : uint8
{
    None = 0 UMETA(Hidden),
    MELEE = 1,
    RANGED = 2,
};
