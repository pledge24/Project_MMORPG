#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Game/Items/P1SlotAddress.h"
#include "Game/Progress/P1ItemIntent.h"
#include "P1ItemActions.generated.h"

class UP1MyPlayerData;

/**
 * 내 플레이어의 아이템 요청 창구.
 *
 * 칸 주소에 대한 의도와 구매를 받아 요청을 보낼지 판정하고, 응답을 기다리는 동안 같은 요청을 막고, 요청을 보낸다.
 * 데이터는 서버 응답으로만 바뀌므로 요청을 보내도 칸과 골드를 고치지 않는다.
 * 내 플레이어 데이터의 하위 객체로만 만든다. 모든 멤버 함수는 게임 스레드 전용이다.
 */
UCLASS()
class P1_API UP1ItemActions : public UObject
{
    GENERATED_BODY()

public:
    /** 보내는 요청의 종류. 응답 대기는 종류마다 따로 건다. */
    enum class ERequestKind : uint8
    {
        Use,
        Equip,
        Unequip,
        Sell,
        Buy,
    };

    /** 요청을 보냈는지, 보내지 않았다면 그 이유. */
    enum class EResult : uint8
    {
        Sent,
        /** 칸이 비었거나 칸이 아니다. 구매라면 아이템 정의가 없다. */
        Empty,
        /** 그 칸의 아이템으로 할 수 없는 의도다. */
        Unusable,
        LevelTooLow,
        CoolingDown,
        NotSellable,
        ShopClosed,
        NotEnoughGold,
        /** 같은 종류의 요청이 응답을 기다리고 있다. */
        Pending,
    };

    /** 보낼 요청 하나. 구매는 TemplateId만, 나머지는 Address와 ItemUid를 쓴다. */
    struct FRequest
    {
        ERequestKind Kind = ERequestKind::Use;
        FP1SlotAddress Address;
        int64 ItemUid = 0;
        int32 TemplateId = 0;
        int32 Count = 0;
    };

    using FRequestSender = TFunction<void(const FRequest&)>;

    //~ Requests
public:
    /** Address의 칸에 Intent를 요청한다. 보내지 않으면 그 이유를 로그로 남긴다. */
    EResult RequestForSlot(EP1ItemIntent Intent, const FP1SlotAddress& Address);

    /** 상점에서 TemplateId 아이템 하나를 산다. */
    EResult RequestBuy(int32 TemplateId);

    /** 판매는 상점 창이 열려 있을 때만 보낸다. 상점 창을 여닫는 쪽이 부른다. */
    void SetShopOpen(bool bOpen) { bShopOpen = bOpen; }

    /** 요청을 보내는 함수를 바꾼다. 테스트가 송신을 기록하려고 쓴다. 비우면 패킷으로 보낸다. */
    void SetRequestSender(FRequestSender InSender) { RequestSender = MoveTemp(InSender); }

private:
    EResult DecideUse(const FP1SlotAddress& Address, OUT FRequest& OutRequest) const;
    EResult DecideUnequip(const FP1SlotAddress& Address, OUT FRequest& OutRequest) const;
    EResult DecideSell(const FP1SlotAddress& Address, OUT FRequest& OutRequest) const;

    /** 응답을 기다리는 같은 종류의 요청이 있으면 보내지 않는다. */
    EResult Send(const FRequest& Request);

    void SendPacket(const FRequest& Request) const;

    bool bShopOpen = false;

    FRequestSender RequestSender;

    //~ Pending
public:
    bool IsPending(ERequestKind Kind) const { return PendingKinds.Contains(Kind); }

    /** Kind의 응답이 오면 성공 여부와 무관하게 부른다. */
    void HandleResponse(ERequestKind Kind) { PendingKinds.Remove(Kind); }

    /** 모든 응답 대기를 푼다. 새로 입장하면 부른다. 끊기기 전에 보낸 요청의 응답은 오지 않는다. */
    void ResetPending() { PendingKinds.Reset(); }

private:
    TSet<ERequestKind> PendingKinds;

    //~ 공통
private:
    UP1MyPlayerData* GetMyPlayerData() const;
};
