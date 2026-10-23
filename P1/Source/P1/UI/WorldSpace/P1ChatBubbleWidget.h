#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/Entities/P1ChatBubbleDisplay.h"
#include "P1ChatBubbleWidget.generated.h"

class UTextBlock;

/**
 * 플레이어 머리 위에 뜨는 말풍선.
 *
 * 그 플레이어가 방금 한 말을 줄임 없이 보여 준다. 언제 보이고 사라질지는 플레이어가 정한다.
 */
UCLASS()
class P1_API UP1ChatBubbleWidget : public UP1UserWidget, public IP1ChatBubbleDisplay
{
    GENERATED_BODY()

    //~ Begin IP1ChatBubbleDisplay Interface
public:
    virtual void SetBubbleMessage(const FText& Message) override;
    virtual void SetBubbleVisible(bool bVisible) override;
    //~ End IP1ChatBubbleDisplay Interface

    //~ Display
protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> MessageText;
};
