#include "UI/P1UserWidget.h"
#include "Core/P1GameInstance.h"

APlayerController* UP1UserWidget::GetP1PlayerController() const
{
    // CreateWidget에 넘긴 소유자를 그대로 돌려준다. GetFirstPlayerController와 달리
    // 위젯을 만든 컨트롤러를 가리키므로, 소유자를 지정하지 않은 위젯에서는 nullptr이 된다.
    return GetOwningPlayer();
}

UP1GameInstance* UP1UserWidget::GetP1GameInstance() const
{
    return Cast<UP1GameInstance>(GetGameInstance());
}
