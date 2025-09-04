// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/ShopWidget.h"
#include "SlotWidget.h"

void UShopWidget::SendBuyPacket(USlotWidget* _Slot)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("OnBuy! template_id: %d"), _Slot->ItemData.TemplateId));
}
