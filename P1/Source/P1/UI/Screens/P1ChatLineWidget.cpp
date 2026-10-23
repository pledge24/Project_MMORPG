#include "UI/Screens/P1ChatLineWidget.h"
#include "Components/RichTextBlock.h"

const TCHAR* const UP1ChatLineWidget::UNKNOWN_NAME = TEXT("알 수 없음");

namespace
{
    /** 글자 스타일 표(DT_ChatTextStyles)의 행 이름. 태그가 없는 글자는 Default 행으로 그린다. */
    const TCHAR* const MINE_TAG = TEXT("Mine");
    const TCHAR* const OTHER_TAG = TEXT("Other");
    const TCHAR* const UNKNOWN_TAG = TEXT("Unknown");
    const TCHAR* const SYSTEM_TAG = TEXT("System");

    // 플레이어가 입력한 글자가 태그로 읽히지 않게 한다. 리치 텍스트 파서가 되돌리는 네 가지이고, &를 먼저 바꿔야 다른 치환이 두 번 바뀌지 않는다.
    FString EscapeMarkup(const FString& Text)
    {
        return Text
            .Replace(TEXT("&"), TEXT("&amp;"))
            .Replace(TEXT("\""), TEXT("&quot;"))
            .Replace(TEXT("<"), TEXT("&lt;"))
            .Replace(TEXT(">"), TEXT("&gt;"));
    }

    FString Tagged(const TCHAR* Tag, const FString& Text)
    {
        return FString::Printf(TEXT("<%s>%s</>"), Tag, *EscapeMarkup(Text));
    }
}

void UP1ChatLineWidget::SetLine(const FP1ChatLine& Line)
{
    LineText->SetText(FText::FromString(MakeMarkup(Line)));
}

FString UP1ChatLineWidget::MakeMarkup(const FP1ChatLine& Line)
{
    if (Line.Kind == EP1ChatLineKind::System)
        return Tagged(SYSTEM_TAG, Line.Message);

    FString NameMarkup;
    if (Line.Name.IsEmpty())
        NameMarkup = Tagged(UNKNOWN_TAG, UNKNOWN_NAME);
    else
        NameMarkup = Tagged(Line.Kind == EP1ChatLineKind::Mine ? MINE_TAG : OTHER_TAG, Line.Name);

    return FString::Printf(TEXT("%s: %s"), *NameMarkup, *EscapeMarkup(Line.Message));
}
