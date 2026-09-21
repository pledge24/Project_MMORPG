#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1WarningTextWidget.generated.h"

/**
 * 
 */
UCLASS()
class P1_API UP1WarningTextWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;

public:
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UI")
    void DisplayWarningMessage(const FText& WarningMessage);

};
