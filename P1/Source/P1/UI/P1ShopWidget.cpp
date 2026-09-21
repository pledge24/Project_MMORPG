#include "UI/P1ShopWidget.h"

#include "Core/P1MyPlayerData.h"
#include "UI/P1SlotWidget.h"
#include "P1.h"
#include "Characters/P1MyPlayer.h"
#include "Core/P1MyPlayerData.h"

void UP1ShopWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
        {
            // MyPlayer 스폰 이벤트에 함수 등록
            MyPlayerData->OnMyPlayerSpawned.AddUObject(this, &UP1ShopWidget::BindMyPlayerSpawned);

            GameInstance->OnRecvBuyItemPkt.AddLambda([this]() { if (IsValid(this)) this->PendingPacket = false; });
        }
    }
}

void UP1ShopWidget::BindMyPlayerSpawned(AP1MyPlayer* MyPlayer)
{
    // MyPlayer->OnRecvBuyItemPkt.AddLambda([this]() { if (IsValid(this)) this->PendingPacket = false; });
}

void UP1ShopWidget::SendBuyItemPacket(UP1SlotWidget* Slot_)
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
