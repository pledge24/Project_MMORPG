// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/HUDWidget.h"
#include "Components/TextBlock.h"
#include "ProgressBarWidget.h"

void UHUDWidget::UpdateAllHUDData(const Protocol::PlayerInfo& _PlayerInfo)
{
    const Protocol::StatInfo& _StatInfo = _PlayerInfo.stat_info();
    UpdateCurLevel(_PlayerInfo.level());
    HpBar->Init(_StatInfo.hp(), _StatInfo.max_hp());
    MpBar->Init(_StatInfo.mp(), _StatInfo.max_mp());
    ExpBar->Init(_PlayerInfo.cur_exp(), _PlayerInfo.max_exp());
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
