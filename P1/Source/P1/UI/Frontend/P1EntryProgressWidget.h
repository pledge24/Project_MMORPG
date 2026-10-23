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
 */
UCLASS()
class P1_API UP1EntryProgressWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Entry Progress
public:
    /** 진행이 없으면 숨기고, 있으면 그 진행의 문구로 화면을 덮는다. */
    void ShowProgress(EP1EntryProgress Progress);

private:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> MessageText;
};
