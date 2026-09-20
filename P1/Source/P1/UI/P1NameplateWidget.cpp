// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/P1NameplateWidget.h"
#include "Characters/P1Monster.h"
#include "Characters/P1Player.h"
#include "UI/P1ProgressBarWidget.h"
#include "Components/TextBlock.h"

void UP1NameplateWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UP1NameplateWidget::InitializeWidget(AActor* Actor)
{
    if (AP1Player* Player = Cast<AP1Player>(Actor))
    {
        InitializePlayerNameplate(Player);
    }
    else if (AP1Monster* Monster = Cast<AP1Monster>(Actor))
    {
        InitializeMonsterNameplate(Monster);
    }
    else
    {
        return;
    }
}

void UP1NameplateWidget::InitializePlayerNameplate(AP1Player* TargetPlayer)
{
    NameTextInitVisibility = ESlateVisibility::SelfHitTestInvisible;
    HpBarInitVisibility = ESlateVisibility::Collapsed;

    NameTextBlock->SetVisibility(NameTextInitVisibility);
    HpBar->SetVisibility(HpBarInitVisibility);

    NameTextBlock->SetText(TargetPlayer->GetPlayerName());
}

void UP1NameplateWidget::InitializeMonsterNameplate(AP1Monster* TargetMonster)
{
    NameTextInitVisibility = ESlateVisibility::SelfHitTestInvisible;
    HpBarInitVisibility = ESlateVisibility::SelfHitTestInvisible;

    NameTextBlock->SetVisibility(NameTextInitVisibility);
    HpBar->SetVisibility(HpBarInitVisibility);

    const FP1MonsterData& MonsterData = TargetMonster->GetMonsterData();
    if (TargetMonster->GetTemplateId() == 0)
        return;

    NameTextBlock->SetText(FText::FromString(MonsterData.MonsterName));

    int64 CurHp = TargetMonster->GetCurHp();
    int64 MaxHp = MonsterData.MaxHp;
    HpBar->Init(CurHp, MaxHp);
}
