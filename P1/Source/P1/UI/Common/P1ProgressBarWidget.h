#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1ProgressBarWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class UTexture2D;

UCLASS()
class P1_API UP1ProgressBarWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    /** 에디터 디자이너에서도 불린다. 텍스처와 자리 문구를 위젯에 입힌다. */
    virtual void NativePreConstruct() override;
    //~ End UUserWidget Interface

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
    void UpdateBar();

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "ProgressBar")
    TObjectPtr<UProgressBar> ProgressBar;

    /** 막대 위에 겹쳐 현재값이나 백분율을 적는다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> TextBlock;

    /** 막대 위에 겹치는 눈금 그림이다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> GridImage;

    /** 켜면 문구를 백분율로 적는다. 끄면 현재값과 최대값을 적는다. */
    bool bIsPercentFormat = false;

    //~ Appearance
protected:
    /** 채워진 부분의 텍스처다. 막대마다 다르다. */
    UPROPERTY(EditAnywhere, Category = "ProgressBar")
    TObjectPtr<UTexture2D> FillTexture;

    /** 비어 있는 부분의 텍스처다. */
    UPROPERTY(EditAnywhere, Category = "ProgressBar")
    TObjectPtr<UTexture2D> BackgroundTexture;

    /** 눈금 그림의 텍스처다. */
    UPROPERTY(EditAnywhere, Category = "ProgressBar")
    TObjectPtr<UTexture2D> GridTexture;

    /** 값이 들어오기 전과 에디터 디자이너에 보이는 문구다. */
    UPROPERTY(EditAnywhere, Category = "ProgressBar")
    FText PlaceholderText;
};
