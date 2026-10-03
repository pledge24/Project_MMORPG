#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/World/P1NameTagDisplay.h"
#include "P1NameTagWidget.generated.h"

class UTextBlock;

/** 포털 위에 뜨는 이름표다. 문구와 색은 포털이 정한다. */
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
