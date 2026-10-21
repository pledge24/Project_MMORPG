#pragma once

#include "CoreMinimal.h"
#include "P1ItemIntent.generated.h"

/** 플레이어가 칸 하나에 대해 하려는 일. 컨테이너 위젯이 클릭을 이것으로 바꿔 아이템 요청 창구에 넘긴다. */
UENUM(BlueprintType)
enum class EP1ItemIntent : uint8
{
    /** 아무것도 하지 않는다. */
    None,
    /** 소모품이면 사용하고 장비면 착용한다. */
    Use,
    /** 착용한 장비를 벗는다. */
    Unequip,
    /** 하나를 상점에 판다. */
    Sell,
};
