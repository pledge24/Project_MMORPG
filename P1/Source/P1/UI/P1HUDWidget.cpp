#include "UI/P1HUDWidget.h"
#include "UI/P1ProgressBarWidget.h"
#include "Components/TextBlock.h"
#include "Characters/P1MyPlayer.h"
#include "Core/P1GameInstance.h"
#include "Core/P1MyPlayerData.h"

void UP1HUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = GetP1GameInstance())
    {
        // MyPlayerData에서 스텟 정보를 가져와 갱신한다.
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
        {
            const Protocol::PlayerInfo& PlayerInfo_ = MyPlayerData->GetPlayerInfo();

            Name_txt->SetText(FText::FromString(UTF8_TO_TCHAR(PlayerInfo_.name().c_str())));
            Level_txt->SetText(FText::AsNumber(PlayerInfo_.level()));
            HpBar->Init(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_HP), MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAX_HP));
            MpBar->Init(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MP), MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAX_MP));
            ExpBar->Init(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_EXP), MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAX_EXP), true);

            // 바인딩 셋업
            MyPlayerData->OnLevelChanged.AddUObject(this, &UP1HUDWidget::UpdateLevel);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAX_HP].AddUObject(this, &UP1HUDWidget::UpdateMaxHp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_HP].AddUObject(this, &UP1HUDWidget::UpdateCurHp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAX_MP].AddUObject(this, &UP1HUDWidget::UpdateMaxMp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MP].AddUObject(this, &UP1HUDWidget::UpdateCurMp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAX_EXP].AddUObject(this, &UP1HUDWidget::UpdateMaxExp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_EXP].AddUObject(this, &UP1HUDWidget::UpdateCurExp);
        }
    }
}

void UP1HUDWidget::UpdateLevel(int32 Level)
{
    Level_txt->SetText(FText::AsNumber(Level));
}


void UP1HUDWidget::UpdateMaxHp(int64 Value)
{
    HpBar->SetMaxValue(Value);
}

void UP1HUDWidget::UpdateCurHp(int64 Value)
{
    HpBar->SetCurValue(Value);
}

void UP1HUDWidget::UpdateHpBar(TOptional<int64> CurValue, TOptional<int64> MaxValue)
{
    if (CurValue.IsSet() && MaxValue.IsSet())
        HpBar->SetBoth(CurValue.GetValue(), MaxValue.GetValue());

    if (CurValue.IsSet())
        HpBar->SetCurValue(CurValue.GetValue());
    else if (MaxValue.IsSet())
        HpBar->SetMaxValue(MaxValue.GetValue());
}

void UP1HUDWidget::UpdateMaxMp(int64 Value)
{
    MpBar->SetMaxValue(Value);
}

void UP1HUDWidget::UpdateCurMp(int64 Value)
{
    MpBar->SetCurValue(Value);
}

void UP1HUDWidget::UpdateMpBar(TOptional<int64> CurValue, TOptional<int64> MaxValue)
{
    if (CurValue.IsSet() && MaxValue.IsSet())
        MpBar->SetBoth(CurValue.GetValue(), MaxValue.GetValue());

    if (CurValue.IsSet())
        MpBar->SetCurValue(CurValue.GetValue());
    else if (MaxValue.IsSet())
        MpBar->SetMaxValue(MaxValue.GetValue());
}

void UP1HUDWidget::UpdateMaxExp(int64 Value)
{
    ExpBar->SetMaxValue(Value);
}

void UP1HUDWidget::UpdateCurExp(int64 Value)
{
    ExpBar->SetCurValue(Value);
}

void UP1HUDWidget::UpdateExpBar(TOptional<int64> CurValue, TOptional<int64> MaxValue)
{
    if(CurValue.IsSet() && MaxValue.IsSet())
        ExpBar->SetBoth(CurValue.GetValue(), MaxValue.GetValue());

    if (CurValue.IsSet())
        ExpBar->SetCurValue(CurValue.GetValue());
    else if(MaxValue.IsSet())
        ExpBar->SetMaxValue(MaxValue.GetValue());
}

void UP1HUDWidget::SetBattleModeTxt(bool battleMode)
{
    if (battleMode)
    {
        FString Message = TEXT("전투 모드: 활성화");
        BattleMode_txt->SetText(FText::FromString(Message));
    }
    else
    {
        FString Message = TEXT("전투 모드: 비활성화");
        BattleMode_txt->SetText(FText::FromString(Message));
    }
}
