#pragma once

#include "CoreMinimal.h"
#include "P1WidgetType.generated.h"

/** 화면 서브시스템이 여닫는 창의 종류다. 블루프린트 핀이 이 이름으로 저장되어 있으므로 이름을 바꾸지 않는다. */
UENUM(BlueprintType)
enum class EP1WidgetType : uint8
{
    WIDGET_NONE = 0 UMETA(Hidden),
    WIDGET_STATUS_WINDOW = 1 UMETA(DisplayName="StatusWindow"),
    WIDGET_INVENTORY = 2 UMETA(DisplayName = "Inventory"),
    WIDGET_SHOP = 3 UMETA(DisplayName = "Shop"),
};
