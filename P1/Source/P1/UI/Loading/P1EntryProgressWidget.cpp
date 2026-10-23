#include "UI/Loading/P1EntryProgressWidget.h"
#include "Components/TextBlock.h"
#include "Core/P1EntryFlowSubsystem.h"

void UP1EntryProgressWidget::SetProgress(EP1EntryProgress Progress)
{
    switch (Progress)
    {
    case EP1EntryProgress::ConnectingWorld:
        MessageText->SetText(FText::FromString(TEXT("월드에 접속하는 중…")));
        break;
    case EP1EntryProgress::LoadingMap:
        MessageText->SetText(FText::FromString(TEXT("맵을 불러오는 중…")));
        break;
    case EP1EntryProgress::None:
        break;
    }
}
