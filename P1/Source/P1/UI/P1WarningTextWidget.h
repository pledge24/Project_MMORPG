#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1WarningTextWidget.generated.h"

UCLASS()
class P1_API UP1WarningTextWidget : public UUserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
public:
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Warning Message
public:
    /** 경고 문구를 띄운다. 구현은 블루프린트에 있다. */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UI")
    void DisplayWarningMessage(const FText& WarningMessage);
};
