#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/World/P1NameTagDisplay.h"
#include "P1NameTagWidget.generated.h"

class UTextBlock;

/**
 * 포털 위에 뜨는 이름표.
 *
 * 월드에 놓인 포털을 플레이어가 알아볼 수 있게 그 이름을 보여 준다.
 */
UCLASS()
class P1_API UP1NameTagWidget : public UP1UserWidget, public IP1NameTagDisplay
{
    GENERATED_BODY()

    //~ Begin IP1NameTagDisplay Interface
public:
    virtual void SetNameTag(const FText& Text, const FLinearColor& Color) override;
    //~ End IP1NameTagDisplay Interface

    //~ Display
protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> NameTag;
};
