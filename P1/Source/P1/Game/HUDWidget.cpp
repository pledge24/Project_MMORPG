// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/HUDWidget.h"
#include "Components/TextBlock.h"
#include "ProgressBarWidget.h"
#include "P1MyPlayer.h"

void UHUDWidget::SetupDelegateBinding(AP1MyPlayer* Player)
{
    if (!Player)
    {
        Player->OnChangedStatInfoDelegate.AddUObject(this, &UHUDWidget::UpdateAllStatInfo);
    }
}

void UHUDWidget::UpdateAllHUDData(const Protocol::PlayerInfo& PlayerInfo_)
{
    const Protocol::StatInfo& StatInfo_ = PlayerInfo_.stat_info();

    UpdateCurLevel(PlayerInfo_.level());
    HpBar->Init(StatInfo_.hp(), StatInfo_.max_hp());
    MpBar->Init(StatInfo_.mp(), StatInfo_.max_mp());
    ExpBar->Init(PlayerInfo_.cur_exp(), PlayerInfo_.max_exp());
}

void UHUDWidget::UpdateAllStatInfo(const Protocol::StatInfo& StatInfo_)
{
    if(StatInfo_.has_hp())
}

void UHUDWidget::UpdateCurLevel(int32 Level)
{
    Level_txt->SetText(FText::AsNumber(Level));
}


void UHUDWidget::UpdateCurHp(int32 Hp)
{
    HpBar->UpdateBar(Hp);
}

void UHUDWidget::UpdateCurMp(int32 Mp)
{
    MpBar->UpdateBar(Mp);
}

void UHUDWidget::UpdateCurExp(int32 Exp)
{
    ExpBar->UpdateBar(Exp, true);
}
