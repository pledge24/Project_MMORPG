#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1WindowLayerWidget.generated.h"

class UCanvasPanel;

/**
 * 여닫는 창을 담는 레이어.
 *
 * 화면의 창들을 한곳에 모아 그리고, 플레이어가 클릭한 창을 다른 창보다 앞에 보이게 한다.
 */
UCLASS()
class P1_API UP1WindowLayerWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;

    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    //~ End UUserWidget Interface

    //~ Windows
public:
    /**
     * 창을 레이어에 넣는다. 창마다 한 번만 넣는다.
     * 창은 뷰포트에 직접 붙였을 때와 같은 자리에 놓인다.
     */
    void AddWindow(UUserWidget* Window);

    /** AddWindow로 넣은 창을 다른 창보다 앞에 그린다. */
    void BringToFront(UUserWidget* Window);

private:
    UPROPERTY()
    TObjectPtr<UCanvasPanel> Canvas;

    /** 마지막으로 앞에 올린 창의 Z 순서. */
    int32 TopZOrder = 0;
};
