#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "P1ChatBubbleDisplay.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UP1ChatBubbleDisplay : public UInterface
{
    GENERATED_BODY()
};

/**
 * 플레이어 머리 위에 띄우는 말풍선 위젯의 창구.
 *
 * 플레이어가 위젯의 구체 타입을 모른 채 말풍선의 문구와 표시 여부를 넘기게 한다.
 * Game/이 UI/를 부르지 않기 위해서다.
 */
class P1_API IP1ChatBubbleDisplay
{
    GENERATED_BODY()

public:
    /** 말풍선에 보일 문구를 입힌다. 보일지는 SetBubbleVisible이 정한다. */
    virtual void SetBubbleMessage(const FText& Message) = 0;

    virtual void SetBubbleVisible(bool bVisible) = 0;
};
