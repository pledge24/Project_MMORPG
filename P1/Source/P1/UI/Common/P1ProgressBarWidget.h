#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1ProgressBarWidget.generated.h"

UCLASS()
class P1_API UP1ProgressBarWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Value
    // 아래 세 함수 모두 최대값이 0 이하이면 막대를 비운다.
public:
    void Init(int64 CurValue, int64 MaxValue, bool IsPercentFormat = false);

    void SetCurValue(int64 Value);
    void SetMaxValue(int64 Value);

protected:
    int64 _CurValue = 0;
    int64 _MaxValue = 0;

    //~ Display
protected:
    /** 막대 옆 문구를 바꾼다. 구현은 블루프린트에 있다. */
    UFUNCTION(BlueprintImplementableEvent, Category = "ProgressBar")
    void SetProgressBarText(const FString& ProgressText);

    void UpdateBar();

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "ProgressBar")
    TObjectPtr<class UProgressBar> ProgressBar;

    /** 켜면 문구를 백분율로 적는다. 끄면 현재값과 최대값을 적는다. */
    bool bIsPercentFormat = false;
};
