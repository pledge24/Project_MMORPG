// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/ShopWidget.h"

#include "MyPlayerData.h"
#include "Widgets/SlotWidget.h"
#include "P1.h"
#include "P1MyPlayer.h"
#include "MyPlayerData.h"

void UShopWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {
        if (UMyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UMyPlayerData>())
        {
            // MyPlayer 스폰 이벤트에 함수 등록
            MyPlayerData->OnMyPlayerSpawned.AddUObject(this, &UShopWidget::BindMyPlayerSpawned);
        }
    }
}

void UShopWidget::BindMyPlayerSpawned(AP1MyPlayer* MyPlayer)
{
    MyPlayer->OnRecvBuyItemPkt.AddLambda([this]() { if (IsValid(this)) this->PendingPacket = false; });
}

void UShopWidget::SendBuyItemPacket(USlotWidget* Slot_)
{
    if (PendingPacket)
        return;
    else
        PendingPacket = true;

    if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {

        int64 Gold = GameInstance->GetMyPlayerData()->GetGold();
        int64 BuyPrice = Slot_->ItemData.BuyPrice;

        if (Gold < BuyPrice)
        {
            return;
        }
            
        Protocol::C_BUY_ITEM pkt;
        pkt.set_template_id(Slot_->ItemData.TemplateId);
        pkt.set_count(1);
        SEND_PACKET(pkt)
    }
}
