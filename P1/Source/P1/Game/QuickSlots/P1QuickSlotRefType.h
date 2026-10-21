#pragma once

#include "CoreMinimal.h"
#include "P1QuickSlotRefType.generated.h"

/** 퀵슬롯이 가리키는 대상의 종류. 서버와 DB의 값과 같다. */
UENUM(BlueprintType)
enum class EP1QuickSlotRefType : uint8
{
    /** 빈 퀵슬롯. */
    None = 0,
    /** 아이템 정의. Id가 아이템 번호다. */
    Item = 1,
};
