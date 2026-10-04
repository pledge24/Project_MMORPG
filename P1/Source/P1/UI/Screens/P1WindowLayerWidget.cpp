#include "UI/Screens/P1WindowLayerWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Layout/WidgetPath.h"

void UP1WindowLayerWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 블루프린트가 없어 위젯 트리가 비어 있다. 슬레이트 위젯을 만들기 전에 루트를 채운다.
    Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WindowCanvas"));
    Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    WidgetTree->RootWidget = Canvas;
}

FReply UP1WindowLayerWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 슬롯 위젯이 클릭을 Handled로 소비하므로 버블링 단계에서는 창까지 올라오지 않는다.
    // 그래서 터널링 단계인 프리뷰에서 먼저 본다.
    // 창의 캔버스 슬롯은 화면 전체를 덮으므로 위치로는 창을 가를 수 없다. 이벤트 경로에 든 창을 찾는다.
    const FWidgetPath* EventPath = InMouseEvent.GetEventPath();
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && EventPath != nullptr && Canvas != nullptr)
    {
        for (UWidget* Child : Canvas->GetAllChildren())
        {
            UUserWidget* Window = Cast<UUserWidget>(Child);
            const TSharedPtr<SWidget> WindowWidget = Window ? Window->GetCachedWidget() : nullptr;
            if (WindowWidget.IsValid() && EventPath->ContainsWidget(WindowWidget.Get()))
            {
                BringToFront(Window);
                break;
            }
        }
    }

    return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UP1WindowLayerWidget::AddWindow(UUserWidget* Window)
{
    if (Window == nullptr || Canvas == nullptr)
        return;

    UCanvasPanelSlot* WindowSlot = Canvas->AddChildToCanvas(Window);
    WindowSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    WindowSlot->SetOffsets(FMargin(0.f));
}

void UP1WindowLayerWidget::BringToFront(UUserWidget* Window)
{
    if (UCanvasPanelSlot* WindowSlot = Cast<UCanvasPanelSlot>(Window ? Window->Slot : nullptr))
        WindowSlot->SetZOrder(++TopZOrder);
}
