#include "UI/Screens/P1ShopWidget.h"

#include "Core/P1MyPlayerData.h"
#include "UI/Common/P1SlotWidget.h"
#include "P1.h"
#include "Network/P1PacketSender.h"
#include "Game/Entities/P1MyPlayer.h"

void UP1ShopWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = GetP1GameInstance())
    {
        GameInstance->OnRecvBuyItemPkt.AddWeakLambda(this, [this]() { PendingPacket = false; });
    }
}

void UP1ShopWidget::SendBuyItemPacket(UP1SlotWidget* Slot_)
{
    // null은 PendingPacket을 올리기 전에 거른다. 올린 뒤에 돌아가면 응답이 오지 않아 구매가 막힌다.
    auto* GameInstance = GetP1GameInstance();
    UP1MyPlayerData* MyPlayerData = GameInstance ? GameInstance->GetMyPlayerData() : nullptr;
    if (Slot_ == nullptr || MyPlayerData == nullptr)
        return;

    if (PendingPacket)
        return;
    else
        PendingPacket = true;

    int64 Gold = MyPlayerData->GetGold();
    int64 BuyPrice = Slot_->ItemData.BuyPrice;

    // TODO: 골드가 모자란 분기는 PendingPacket을 올린 채 돌아가 구매를 막는다. #132가 고친다.
    if (Gold < BuyPrice)
    {
        return;
    }

    Protocol::C_BUY_ITEM pkt;
    pkt.set_template_id(Slot_->ItemData.TemplateId);
    pkt.set_count(1);
    FP1PacketSender::Send(this, pkt);
}
