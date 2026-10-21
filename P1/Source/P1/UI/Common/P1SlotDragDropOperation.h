#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "UI/Common/P1SlotDragPayload.h"
#include "P1SlotDragDropOperation.generated.h"

/**
 * 슬롯 위젯 하나를 끌어 옮기는 조작.
 *
 * 끌기를 시작한 슬롯이 무엇을 그리던 자리인지 싣고 다닌다. 무엇을 요청할지는 놓은 자리의 위젯이 정한다.
 */
UCLASS()
class P1_API UP1SlotDragDropOperation : public UDragDropOperation
{
    GENERATED_BODY()

public:
    FP1SlotDragPayload Payload;
};
