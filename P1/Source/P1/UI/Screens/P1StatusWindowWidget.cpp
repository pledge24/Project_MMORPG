#include "UI/Screens/P1StatusWindowWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/P1GameInstance.h"
#include "Game/Progress/P1MyPlayerData.h"

void UP1StatusWindowWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 팁 버튼 연결.
    Button_Details->OnClicked.AddUniqueDynamic(this, &UP1StatusWindowWidget::ToggleTips);

    if (auto* GameInstance = GetP1GameInstance())
    {
        // 지금 능력치로 채운다.
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
        {
            UpdateAllStat(MyPlayerData);

            // 이후 바뀌는 값을 구독.
            // 창은 한 번 만들고 표시 여부만 바꾸므로, 수치도 열 때 다시 읽지 않고 바뀔 때마다 받는다.
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAX_HP].AddUObject(this, &UP1StatusWindowWidget::UpdateMaxHp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAX_MP].AddUObject(this, &UP1StatusWindowWidget::UpdateMaxMp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_PHYSICAL_ATTACK].AddUObject(this, &UP1StatusWindowWidget::UpdatePhysicalAttack);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAGICAL_ATTACK].AddUObject(this, &UP1StatusWindowWidget::UpdateMagicalAttack);
        }
    }

}

void UP1StatusWindowWidget::UpdateAllStat(UP1MyPlayerData* MyPlayerData)
{
    UpdateMaxHp(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAX_HP));
    UpdateMaxMp(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAX_MP));
    UpdatePhysicalAttack(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK));
    UpdateMagicalAttack(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK));
}

void UP1StatusWindowWidget::UpdateMaxHp(int64 Value)
{
    Details_MaxHp->SetText(FText::AsNumber(Value));
}

void UP1StatusWindowWidget::UpdateMaxMp(int64 Value)
{
    Details_MaxMp->SetText(FText::AsNumber(Value));
}

void UP1StatusWindowWidget::UpdatePhysicalAttack(int64 Value)
{
    Details_Physical_Attack->SetText(FText::AsNumber(Value));
}

void UP1StatusWindowWidget::UpdateMagicalAttack(int64 Value)
{
    Details_Magical_Attack->SetText(FText::AsNumber(Value));
}

void UP1StatusWindowWidget::ToggleTips()
{
    CanvasPanel_Tips->SetVisibility(
        CanvasPanel_Tips->IsVisible() ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
}
