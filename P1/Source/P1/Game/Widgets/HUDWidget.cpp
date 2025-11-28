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
            const Protocol::StatInfo& StatInfo_ = MyPlayerData->GetStatInfo();

            Name_txt->SetText(FText::FromString(UTF8_TO_TCHAR(PlayerInfo_.name().c_str())));
            Level_txt->SetText(FText::AsNumber(PlayerInfo_.level()));
            HpBar->Init(StatInfo_.hp(), StatInfo_.max_hp());
            MpBar->Init(StatInfo_.mp(), StatInfo_.max_mp());
            ExpBar->Init(PlayerInfo_.cur_exp(), PlayerInfo_.max_exp(), true);

            // MyPlayer 스폰 이벤트에 함수 등록
            MyPlayerData->OnMyPlayerSpawned.AddUObject(this, &UHUDWidget::BindMyPlayerSpawned);
        }
    }
}

void UHUDWidget::BindMyPlayerSpawned(AP1MyPlayer* MyPlayer)
{
    // 바인딩 셋업
    MyPlayer->OnLevelChanged.AddUObject(this, &UHUDWidget::UpdateLevel);
    MyPlayer->OnExpChanged.AddUObject(this, &UHUDWidget::UpdateExpBar);
    MyPlayer->OnStatInfoChanged.AddUObject(this, &UHUDWidget::UpdateAllStatsChanged);
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
