#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1WindowLayerWidget.generated.h"

class UCanvasPanel;

/**
 * 여닫는 창을 담는 레이어다. 뷰포트에는 이 레이어만 한 번 붙고, 창은 이 안에 한 번만 들어간다.
 * 창을 앞으로 올려도 창을 뗐다 붙이지 않으므로 창의 NativeConstruct가 다시 돌지 않는다.
 * 블루프린트 없이 C++ 클래스로 만든다.
 */
UCLASS()
class P1_API UP1WindowLayerWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    /** 위젯 트리의 루트 캔버스를 만든다. */
    virtual void NativeOnInitialized() override;

    /** 좌클릭이 닿은 창을 앞으로 올린다. 클릭은 소비하지 않으므로 창 안의 위젯이 그대로 받는다. */
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    //~ End UUserWidget Interface

    //~ Windows
public:
    /** 창을 화면 전체에 맞춘 슬롯으로 넣는다. 창의 배치는 뷰포트에 직접 붙였을 때와 같다. */
    void AddWindow(UUserWidget* Window);

    /** 이 레이어에 넣은 창을 다른 창보다 앞에 그린다. */
    void BringToFront(UUserWidget* Window);

private:
    UPROPERTY()
    TObjectPtr<UCanvasPanel> Canvas;

    /** 마지막으로 앞에 올린 창의 Z 순서다. 올릴 때마다 1씩 늘린다. */
    int32 TopZOrder = 0;
};
