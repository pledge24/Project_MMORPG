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
        InitializePlayerNameplate(Player);
    }
    else if (AMonster* Monster = Cast<AMonster>(Actor))
    {
        InitializeMonsterNameplate(Monster);
    }
    else
    {
        return;
    }
}

void UNameplateWidget::InitializePlayerNameplate(AP1Player* TargetPlayer)
{
    NameTextInitVisibility = ESlateVisibility::SelfHitTestInvisible;
    HpBarInitVisibility = ESlateVisibility::Collapsed;

    NameTextBlock->SetVisibility(NameTextInitVisibility);
    HpBar->SetVisibility(HpBarInitVisibility);

    NameTextBlock->SetText(TargetPlayer->GetPlayerName());
}

void UNameplateWidget::InitializeMonsterNameplate(AMonster* TargetMonster)
{
    NameTextInitVisibility = ESlateVisibility::SelfHitTestInvisible;
    HpBarInitVisibility = ESlateVisibility::SelfHitTestInvisible;

    NameTextBlock->SetVisibility(NameTextInitVisibility);
    HpBar->SetVisibility(HpBarInitVisibility);

    const FMonsterData& MonsterData = TargetMonster->GetMonsterData();
    if (TargetMonster->GetTemplateId() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("InitializeMonsterNameplate"));
        return;
    }

    NameTextBlock->SetText(FText::FromString(MonsterData.MonsterName));

    int64 CurHp = TargetMonster->GetCurHp();
    int64 MaxHp = MonsterData.MaxHp;
    HpBar->Init(CurHp, MaxHp);
}
