// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/ShopWidget.h"
#include "SlotWidget.h"
#include "P1.h"
#include "P1MyPlayer.h"

void UShopWidget::SendBuyPacket(USlotWidget* _Slot)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("OnBuy! template_id: %d"), _Slot->ItemData.TemplateId));

    if (UP1GameInstance* GameInstance = Cast<UP1GameInstance>(UGameplayStatics::GetGameInstance(GetWorld())))
    {
        if (AP1MyPlayer* MyPlayer = Cast<AP1MyPlayer>(GameInstance->MyPlayer))
        {
            int64 Gold = MyPlayer->GetGold();
            int64 BuyPrice = _Slot->ItemData.BuyPrice;

            if (Gold < BuyPrice)
                return;
            
            Protocol::C_BUY_ITEM pkt;
            pkt.set_template_id(_Slot->ItemData.TemplateId);
            pkt.set_count(1);
            SEND_PACKET(pkt)
        }

    }
}
