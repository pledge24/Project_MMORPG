#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1WarningTextWidget.generated.h"

class UTextBlock;

/**
 * 화면에 잠깐 떴다 사라지는 경고 문구.
 *
 * 플레이어에게 알릴 경고를 한 번에 하나씩, 정해진 시간 동안만 보여 준다.
 */
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
    /**
     * 경고 문구를 DisplaySeconds 동안 띄운다.
     * 보이는 중에 다시 부르면 문구를 바꾸고 시간을 처음부터 다시 센다.
     */
    void DisplayWarningMessage(const FText& WarningMessage);

private:
    void HideWarningMessage();

    static constexpr float DisplaySeconds = 3.f;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> WarningText;

    FTimerHandle HideTimerHandle;
};
