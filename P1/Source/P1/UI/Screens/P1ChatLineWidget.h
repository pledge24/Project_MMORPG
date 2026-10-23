#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/Chat/P1ChatLine.h"
#include "P1ChatLineWidget.generated.h"

class URichTextBlock;

/**
 * 채팅창의 한 줄.
 *
 * 말한 플레이어의 이름과 메시지를 한 줄로 보여 준다. 이름의 색은 누가 말했는지에 따라 WBP의 글자 스타일이 정한다.
 */
UCLASS()
class P1_API UP1ChatLineWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Line
public:
    void SetLine(const FP1ChatLine& Line);

    /** 리치 텍스트의 태그로 쓸 글자를 이스케이프해 Line을 표시할 마크업으로 만든다. 태그 이름은 WBP 글자 스타일 표의 행 이름이다. */
    static FString MakeMarkup(const FP1ChatLine& Line);

    /** 이름을 받지 못한 줄에 대신 보여 주는 이름. */
    static const TCHAR* const UNKNOWN_NAME;

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<URichTextBlock> LineText;
};
