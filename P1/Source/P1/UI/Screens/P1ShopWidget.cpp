#include "UI/Screens/P1ShopWidget.h"

#include "Game/Progress/P1MyPlayerData.h"
#include "UI/Common/P1SlotWidget.h"
#include "Components/PanelWidget.h"
#include "Network/P1PacketSender.h"
#include "Core/P1GameInstance.h"
#include "Utils/LogCategory.h"

void UP1ShopWidget::NativeConstruct()
{
    Super::NativeConstruct();

    for (UWidget* Child : UGP_Shop->GetAllChildren())
    {
        if (UP1SlotWidget* SlotWidget = Cast<UP1SlotWidget>(Child))
            SlotWidget->OnRightClicked.AddUObject(this, &UP1ShopWidget::HandleSlotRightClicked);
    }

    if (auto* GameInstance = GetP1GameInstance())
    {
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
            MyPlayerData->OnRecvBuyItemPkt.AddWeakLambda(this, [this]() { PendingPacket = false; });
    }
}

void UP1ShopWidget::HandleSlotRightClicked(UP1SlotWidget* SlotWidget)
{
    UP1GameInstance* GameInstance = GetP1GameInstance();
    UP1MyPlayerData* MyPlayerData = GameInstance ? GameInstance->GetSubsystem<UP1MyPlayerData>() : nullptr;
    if (MyPlayerData == nullptr || PendingPacket)
        return;

    // 보내기 전에 돌아가는 분기를 모두 지난 뒤에 대기를 켠다. 켜고 보내지 않으면 응답이 오지 않아 구매가 막힌다.
    // 빈 진열 칸은 가격이 0이라 골드 검사를 통과하므로 먼저 가린다.
    if (SlotWidget->ItemData.TemplateId <= 0)
    {
        UE_LOG(LogP1UI, Log, TEXT("빈 진열 칸은 살 수 없다."));
        return;
    }

    if (MyPlayerData->GetGold() < SlotWidget->ItemData.BuyPrice)
    {
        UE_LOG(LogP1UI, Log, TEXT("골드가 모자라 살 수 없다. 템플릿 %d"), SlotWidget->ItemData.TemplateId);
        return;
    }

    Protocol::C_BUY_ITEM Pkt;
    Pkt.set_template_id(SlotWidget->ItemData.TemplateId);
    Pkt.set_count(1);

    PendingPacket = true;
    FP1PacketSender::Send(this, Pkt);
}
