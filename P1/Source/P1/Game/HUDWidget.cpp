// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/HUDWidget.h"
#include "ProgressBarWidget.h"

void UHUDWidget::UpdateAllProgressBar(const Protocol::StatInfo _StatInfo, int32 CurExp, int32 MaxExp)
{
    HpBar->Init(_StatInfo.max_hp(), _StatInfo.hp());
    MpBar->Init(_StatInfo.max_mp(), _StatInfo.mp());
    ExpBar->Init(CurExp, MaxExp);
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
