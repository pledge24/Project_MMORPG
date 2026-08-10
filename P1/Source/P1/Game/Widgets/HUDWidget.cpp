// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUDWidget.h"
#include "Widgets/ProgressBarWidget.h"
#include "Components/TextBlock.h"
#include "P1MyPlayer.h"
#include "P1GameInstance.h"
#include "MyPlayerData.h"

void UHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {
        // MyPlayerData에서 스텟 정보를 가져와 갱신한다.
        if (UMyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UMyPlayerData>())
        {
            const Protocol::PlayerInfo& PlayerInfo_ = MyPlayerData->GetPlayerInfo();

            Name_txt->SetText(FText::FromString(UTF8_TO_TCHAR(PlayerInfo_.name().c_str())));
            Level_txt->SetText(FText::AsNumber(PlayerInfo_.level()));
            HpBar->Init(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_HP), MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAX_HP));
            MpBar->Init(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MP), MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAX_MP));
            ExpBar->Init(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_EXP), MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAX_EXP), true);

            // 바인딩 셋업
            MyPlayerData->OnLevelChanged.AddUObject(this, &UHUDWidget::UpdateLevel);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAX_HP].AddUObject(this, &UHUDWidget::UpdateMaxHp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_HP].AddUObject(this, &UHUDWidget::UpdateCurHp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAX_MP].AddUObject(this, &UHUDWidget::UpdateMaxMp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MP].AddUObject(this, &UHUDWidget::UpdateCurMp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAX_EXP].AddUObject(this, &UHUDWidget::UpdateMaxExp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_EXP].AddUObject(this, &UHUDWidget::UpdateCurExp);
        }
    }
}

void UHUDWidget::UpdateLevel(int32 Level)
{
    Level_txt->SetText(FText::AsNumber(Level));
}


void UHUDWidget::UpdateMaxHp(int64 Value)
{
    HpBar->SetMaxValue(Value);
}

void UHUDWidget::UpdateCurHp(int64 Value)
{
    HpBar->SetCurValue(Value);
}

void UHUDWidget::UpdateHpBar(TOptional<int64> CurValue, TOptional<int64> MaxValue)
{
    if (CurValue.IsSet() && MaxValue.IsSet())
        HpBar->SetBoth(CurValue.GetValue(), MaxValue.GetValue());

    if (CurValue.IsSet())
        HpBar->SetCurValue(CurValue.GetValue());
    else if (MaxValue.IsSet())
        HpBar->SetMaxValue(MaxValue.GetValue());
}

void UHUDWidget::UpdateMaxMp(int64 Value)
{
    MpBar->SetMaxValue(Value);
}

void UHUDWidget::UpdateCurMp(int64 Value)
{
    MpBar->SetCurValue(Value);
}

void UHUDWidget::UpdateMpBar(TOptional<int64> CurValue, TOptional<int64> MaxValue)
{
    if (CurValue.IsSet() && MaxValue.IsSet())
        MpBar->SetBoth(CurValue.GetValue(), MaxValue.GetValue());

    if (CurValue.IsSet())
        MpBar->SetCurValue(CurValue.GetValue());
    else if (MaxValue.IsSet())
        MpBar->SetMaxValue(MaxValue.GetValue());
}

void UHUDWidget::UpdateMaxExp(int64 Value)
{
    ExpBar->SetMaxValue(Value);
}

void UHUDWidget::UpdateCurExp(int64 Value)
{
    ExpBar->SetCurValue(Value);
}

void UHUDWidget::UpdateExpBar(TOptional<int64> CurValue, TOptional<int64> MaxValue)
{
    if(CurValue.IsSet() && MaxValue.IsSet())
        ExpBar->SetBoth(CurValue.GetValue(), MaxValue.GetValue());

    if (CurValue.IsSet())
        ExpBar->SetCurValue(CurValue.GetValue());
    else if(MaxValue.IsSet())
        ExpBar->SetMaxValue(MaxValue.GetValue());
}

void UHUDWidget::SetBattleModeTxt(bool battleMode)
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
