// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Widgets/NameplateWidget.h"
#include "Objects/Monster.h"
#include "Objects/P1Player.h"
#include "ProgressBarWidget.h"
#include "Components/TextBlock.h"

void UNameplateWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UNameplateWidget::InitializeWidget(AActor* Actor)
{
    if (AP1Player* Player = Cast<AP1Player>(Actor))
    {
        NameTextInitVisibility = ESlateVisibility::Visible;
        HpBarInitVisibility = ESlateVisibility::Collapsed;

        NameTextBlock->SetVisibility(NameTextInitVisibility);
        HpBar->SetVisibility(HpBarInitVisibility);

        NameTextBlock->SetText(Player->GetPlayerName());
    }
    else if (AMonster* Monster = Cast<AMonster>(Actor))
    {
        NameTextInitVisibility = ESlateVisibility::Visible;
        HpBarInitVisibility = ESlateVisibility::Visible;

        NameTextBlock->SetVisibility(NameTextInitVisibility);
        HpBar->SetVisibility(HpBarInitVisibility);

        const FMonsterData& MonsterData = Monster->GetMonsterData();
        if (MonsterData.TemplateId == 0)
            return;

        NameTextBlock->SetText(FText::FromString(MonsterData.MonsterName));

        int64 CurHp = Monster->GetCurHp();
        int64 MaxHp = MonsterData.MaxHp;
        HpBar->Init(CurHp, MaxHp);
    }
    else
    {
        return;
    }
}
