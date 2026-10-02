#include "UI/Screens/P1WindowLayerWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

void UP1WindowLayerWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 블루프린트가 없어 위젯 트리가 비어 있다. 슬레이트 위젯을 만들기 전에 루트를 채운다.
    Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WindowCanvas"));
    Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    WidgetTree->RootWidget = Canvas;
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
