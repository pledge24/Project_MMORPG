#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1EntryProgressWidget.generated.h"

class UTextBlock;
enum class EP1EntryProgress : uint8;

/**
 * 접속 진행 표시.
 *
 * 월드에 접속하거나 맵을 불러오는 동안 화면 전체를 덮어 진행 문구를 보여 준다.
 * 언제 띄우고 내릴지는 로딩 화면 서브시스템(UP1LoadingScreenSubsystem)이 정한다. 이 위젯은 문구만 바꾼다.
 */
UCLASS()
class P1_API UP1EntryProgressWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Entry Progress
public:
    /** Progress의 문구를 보인다. 진행이 없으면 문구를 그대로 둔다. 사라지는 동안에도 앞 문구가 보여야 한다. */
    void SetProgress(EP1EntryProgress Progress);

private:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> MessageText;
};
