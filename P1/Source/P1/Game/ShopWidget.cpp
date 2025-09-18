// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/ShopWidget.h"
#include "SlotWidget.h"
#include "P1.h"
#include "P1MyPlayer.h"

void UShopWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {
        // 바인딩 셋업
        GameInstance->OnRep_BuyItem.AddLambda([this]() { if (IsValid(this)) PendingPacket = false; });
    }
}

void UShopWidget::SendBuyItemPacket(USlotWidget* _Slot)
{
    if (PendingPacket)
        return;
    else
        PendingPacket = true;

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("OnBuy! template_id: %d"), _Slot->ItemData.TemplateId));

    if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {
        int64 Gold = GameInstance->GetGold();
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
