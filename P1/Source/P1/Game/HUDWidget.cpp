// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/HUDWidget.h"
#include "Components/TextBlock.h"
#include "ProgressBarWidget.h"
#include "P1MyPlayer.h"
#include "P1GameInstance.h"

void UHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetupDelegateBinding();
}

void UHUDWidget::SetupDelegateBinding()
{
    UWorld* World = GetWorld();
    AP1MyPlayer* MyPlayer = nullptr;
    if (World)
    {
        UP1GameInstance* GameInstance = Cast<UP1GameInstance>(World->GetGameInstance());

        if (GameInstance)
        {
            MyPlayer = Cast<AP1MyPlayer>(GameInstance->MyPlayer);
        }
    }

    if (MyPlayer)
    {
        MyPlayer->OnLevelChanged.AddUObject(this, &UHUDWidget::UpdateCurLevel);
        MyPlayer->OnExpChanged.AddUObject(this, &UHUDWidget::UpdateExp);
        MyPlayer->OnStatInfoChanged.AddUObject(this, &UHUDWidget::UpdateAllStatsChanged);
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

void UHUDWidget::UpdateAllStatsChanged(const Protocol::StatInfo& StatInfo_)
{
    if (StatInfo_.has_hp())
        UpdateCurHp(StatInfo_.hp());
    if (StatInfo_.has_mp())
        UpdateCurMp(StatInfo_.mp());
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

void UHUDWidget::UpdateExp(int32 CurExp, int32 MaxExp)
{
    if (MaxExp > 0)
        ExpBar->Init(CurExp, MaxExp, true);
    else
        ExpBar->UpdateBar(CurExp, true);
}
