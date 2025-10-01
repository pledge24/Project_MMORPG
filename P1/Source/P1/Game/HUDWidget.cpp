// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/HUDWidget.h"
#include "Components/TextBlock.h"
#include "ProgressBarWidget.h"
#include "P1MyPlayer.h"
#include "P1GameInstance.h"
#include "P1.h"

void UHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {
        // Init
        const Protocol::PlayerInfo& PlayerInfo_ = GameInstance->GetPlayerInfo();
        const Protocol::StatInfo& StatInfo_ = PlayerInfo_.stat_info();

        Name_txt->SetText(FText::FromString(UTF8_TO_TCHAR(PlayerInfo_.name().c_str())));
        UpdateLevel(PlayerInfo_.level());
        HpBar->Init(StatInfo_.hp(), StatInfo_.max_hp());
        MpBar->Init(StatInfo_.mp(), StatInfo_.max_mp());
        ExpBar->Init(PlayerInfo_.cur_exp(), PlayerInfo_.max_exp(), true);

        // 바인딩 셋업
        GameInstance->OnLevelChanged.AddUObject(this, &UHUDWidget::UpdateLevel);
        GameInstance->OnExpChanged.AddUObject(this, &UHUDWidget::UpdateExpBar);
        GameInstance->OnStatInfoChanged.AddUObject(this, &UHUDWidget::UpdateAllStatsChanged);
    }
}

void UHUDWidget::UpdateAllStatsChanged(const Protocol::StatInfo& StatInfo_)
{
    /** HP Bar*/
    {
        TOptional<int32> CurValue = StatInfo_.has_hp() ? TOptional<int32>(StatInfo_.hp()) : NullOpt;
        TOptional<int32> MaxValue = StatInfo_.has_max_hp() ? TOptional<int32>(StatInfo_.max_hp()) : NullOpt;
        UpdateHpBar(CurValue, MaxValue);
    }

    /** MP Bar*/
    {
        TOptional<int32> CurValue = StatInfo_.has_mp() ? TOptional<int32>(StatInfo_.mp()) : NullOpt;
        TOptional<int32> MaxValue = StatInfo_.has_max_mp() ? TOptional<int32>(StatInfo_.max_mp()) : NullOpt;
        UpdateMpBar(CurValue, MaxValue);
    }
}

void UHUDWidget::UpdateLevel(int32 Level)
{
    Level_txt->SetText(FText::AsNumber(Level));
}


void UHUDWidget::UpdateMaxHp(int32 Value)
{
    HpBar->SetMaxValue(Value);
}

void UHUDWidget::UpdateCurHp(int32 Value)
{
    HpBar->SetCurValue(Value);
}

void UHUDWidget::UpdateHpBar(TOptional<int32> CurValue, TOptional<int32> MaxValue)
{
    if (CurValue.IsSet() && MaxValue.IsSet())
        HpBar->SetBoth(CurValue.GetValue(), MaxValue.GetValue());

    if (CurValue.IsSet())
        HpBar->SetCurValue(CurValue.GetValue());
    else if (MaxValue.IsSet())
        HpBar->SetMaxValue(MaxValue.GetValue());
}

void UHUDWidget::UpdateMaxMp(int32 Value)
{
    MpBar->SetMaxValue(Value);
}

void UHUDWidget::UpdateCurMp(int32 Value)
{
    MpBar->SetCurValue(Value);
}

void UHUDWidget::UpdateMpBar(TOptional<int32> CurValue, TOptional<int32> MaxValue)
{
    if (CurValue.IsSet() && MaxValue.IsSet())
        MpBar->SetBoth(CurValue.GetValue(), MaxValue.GetValue());

    if (CurValue.IsSet())
        MpBar->SetCurValue(CurValue.GetValue());
    else if (MaxValue.IsSet())
        MpBar->SetMaxValue(MaxValue.GetValue());
}

void UHUDWidget::UpdateExpBar(TOptional<int32> CurValue, TOptional<int32> MaxValue)
{
    if(CurValue.IsSet() && MaxValue.IsSet())
        ExpBar->SetBoth(CurValue.GetValue(), MaxValue.GetValue());

    if (CurValue.IsSet())
        ExpBar->SetCurValue(CurValue.GetValue());
    else if(MaxValue.IsSet())
        ExpBar->SetMaxValue(MaxValue.GetValue());
}
