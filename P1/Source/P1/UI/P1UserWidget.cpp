#include "UI/P1UserWidget.h"
#include "Core/P1GameInstance.h"
#include "Game/Progress/P1MyPlayerData.h"

void UP1UserWidget::NativeDestruct()
{
    // 위젯은 레벨과 함께 사라지고 NativeConstruct에서 다시 붙는다. 여기서 떼지 않으면
    // 맵을 옮길 때마다 레벨보다 오래 사는 델리게이트에 죽은 위젯의 항목이 쌓인다.
    if (UP1GameInstance* GameInstance = GetP1GameInstance())
    {
        GameInstance->RemovePacketListener(this);

        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
            MyPlayerData->RemoveListener(this);
    }

    Super::NativeDestruct();
}

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
