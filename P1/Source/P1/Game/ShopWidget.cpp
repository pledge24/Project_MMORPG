// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/ShopWidget.h"
#include "SlotWidget.h"
#include "P1.h"
#include "P1MyPlayer.h"

void UShopWidget::NativeConstruct()
{
    Super::NativeConstruct();

    auto* PC = UGameplayStatics::GetPlayerController(this, 0);
    AP1MyPlayer* MyPlayer = Cast<AP1MyPlayer>(PC->GetPawn());

    if (MyPlayer)
    {
        // 바인딩 셋업
        MyPlayer->OnRep_BuyItem.AddLambda([this]() { if (IsValid(this)) CanInteractive = true; });
    }
}

void UShopWidget::SendBuyItemPacket(USlotWidget* _Slot)
{
    if (!CanInteractive)
        return;
    else
        CanInteractive = false;

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("OnBuy! template_id: %d"), _Slot->ItemData.TemplateId));

    auto* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (AP1MyPlayer* MyPlayer = Cast<AP1MyPlayer>(PC->GetPawn()))
    {
        int64 Gold = MyPlayer->GetGold();
        int64 BuyPrice = _Slot->ItemData.BuyPrice;

        if (Gold < BuyPrice)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("NO MONEY")));
            return;
        }
            
        Protocol::C_BUY_ITEM pkt;
        pkt.set_template_id(_Slot->ItemData.TemplateId);
        pkt.set_count(1);
        SEND_PACKET(pkt)
    }
}
