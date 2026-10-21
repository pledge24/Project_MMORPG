#pragma once

#include "CoreMinimal.h"
#include "P1ContainerType.generated.h"

/** 아이템을 담는 컨테이너의 종류. 인벤토리의 구역 셋과 착용 장비다. */
UENUM(BlueprintType)
enum class EP1ContainerType : uint8
{
    None = 0,
    InventoryGear = 1,
    InventoryConsumable = 2,
    InventoryMisc = 3,
    /** 칸 번호가 장비 부위(GearType) 값이다. */
    Equipped = 4,
};
