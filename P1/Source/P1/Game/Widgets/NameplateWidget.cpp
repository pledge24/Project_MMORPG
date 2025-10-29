// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Widgets/NameplateWidget.h"
#include "P1Player.h"
#include "Monster.h"
#include "ProgressBarWidget.h"
#include "Components/TextBlock.h"

void UNameplateWidget::NativeConstruct()
{
    Super::NativeConstruct();

    NameTextBlock->SetVisibility(NameTextVisibility);
    HpBar->SetVisibility(HpBarVisibility);
}

void UNameplateWidget::InitializeWidget(AActor* Actor)
{
    if (AP1Player* Player = Cast<AP1Player>(Actor))
    {
        UE_LOG(LogTemp, Warning, TEXT("Player"));

        NameTextVisibility = ESlateVisibility::Visible;

        NameTextBlock->SetText(FText::FromString("AXSXSX"));
        //NameTextBlock->SetText(Player->GetPlayerName());
    }
    else if (AMonster* Monster = Cast<AMonster>(Actor))
    {
        UE_LOG(LogTemp, Warning, TEXT("Monster"));

        NameTextVisibility = ESlateVisibility::Visible;
        HpBarVisibility = ESlateVisibility::Visible;


        const FMonsterData& MonsterData = Monster->GetMonsterData();
        if (MonsterData.TemplateId == 0)
            return;

        NameTextVisibility = ESlateVisibility::Visible;
        NameTextBlock->SetText(FText::FromString(MonsterData.MonsterName));

        HpBarVisibility = ESlateVisibility::Visible;
        int64 CurHp = Monster->GetCurHp();
        int64 MaxHp = MonsterData.MaxHp;
        HpBar->Init(CurHp, MaxHp);
    }
    else
    {
        return;
    }
}
