#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1WarningTextWidget.generated.h"

class UTextBlock;

UCLASS()
class P1_API UP1WarningTextWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
public:
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Warning Message
public:
    /** 경고 문구를 띄우고 DisplaySeconds 뒤에 숨긴다. 보이는 중에 새 경고가 오면 문구를 바꾸고 처음부터 다시 센다. */
    void DisplayWarningMessage(const FText& WarningMessage);

private:
    void HideWarningMessage();

    static constexpr float DisplaySeconds = 3.f;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> WarningText;

    FTimerHandle HideTimerHandle;
};
