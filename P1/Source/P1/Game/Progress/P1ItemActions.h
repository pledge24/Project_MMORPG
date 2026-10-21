#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Game/Items/P1Slot.h"
#include "Game/Items/P1SlotAddress.h"
#include "Game/Progress/P1ItemIntent.h"
#include "Game/QuickSlots/P1QuickSlotRef.h"
#include "P1ItemActions.generated.h"

class UP1MyPlayerData;

/**
 * 내 플레이어의 아이템 요청 창구.
 *
 * 칸 주소에 대한 의도와 구매, 퀵슬롯 등록과 사용을 요청으로 바꾸는 유일한 창구다. 요청할 수 있는지의 판정과 응답 대기를 여기서만 다룬다.
 * 데이터는 서버 응답으로만 바뀌므로 요청을 보내도 칸과 골드를 고치지 않는다.
 * 내 플레이어 데이터의 하위 객체로만 만든다. 모든 멤버 함수는 게임 스레드 전용이다.
 */
UCLASS()
class P1_API UP1ItemActions : public UObject
{
    GENERATED_BODY()

public:
    /** 보내는 요청의 종류. 인벤토리 칸의 사용, 착용, 판매와 퀵슬롯 사용은 응답 대기 하나를 함께 쓰고, 나머지는 각자 쓴다. */
    enum class ERequestKind : uint8
    {
        Use,
        Equip,
        Unequip,
        Sell,
        Buy,
        SetQuickSlot,
        UseQuickSlot,
    };

    /** 요청을 보냈는지, 보내지 않았다면 그 이유. */
    enum class EResult : uint8
    {
        Sent,
        /** 칸이 비었거나 칸이 아니다. 구매라면 아이템 정의가 없고, 퀵슬롯 사용이라면 등록이 없거나 그 소모품이 없다. */
        Empty,
        /** 그 칸의 아이템으로 할 수 없는 의도다. */
        Unusable,
        LevelTooLow,
        CoolingDown,
        NotSellable,
        ShopClosed,
        NotEnoughGold,
        /** 응답 대기를 함께 쓰는 요청이 응답을 기다리고 있다. */
        Pending,
    };

    /**
     * 보낼 요청 하나. 구매는 TemplateId만, 퀵슬롯 등록은 QuickSlotIndex와 QuickSlotRef만, 퀵슬롯 사용은 QuickSlotIndex만,
     * 나머지는 Address와 ItemUid를 쓴다.
     */
    struct FRequest
    {
        ERequestKind Kind = ERequestKind::Use;
        FP1SlotAddress Address;
        int64 ItemUid = 0;
        int32 TemplateId = 0;
        int32 Count = 0;
        int32 QuickSlotIndex = 0;
        FP1QuickSlotRef QuickSlotRef;
    };

    using FRequestSender = TFunction<void(const FRequest&)>;

    //~ Requests
public:
    /** Address의 칸에 Intent를 요청한다. 보내지 않으면 그 이유를 로그로 남긴다. */
    EResult RequestForSlot(EP1ItemIntent Intent, const FP1SlotAddress& Address);

    /** 상점에서 TemplateId 아이템 하나를 산다. */
    EResult RequestBuy(int32 TemplateId);

    /**
     * QuickSlotIndex 퀵슬롯에 Ref를 등록한다. 빈 참조면 해제한다. 다른 퀵슬롯에 있는 대상이면 서버가 둘을 맞바꾼다.
     * 소모품만 등록하고 소지 여부는 보지 않는다.
     */
    EResult RequestSetQuickSlot(int32 QuickSlotIndex, const FP1QuickSlotRef& Ref);

    /** QuickSlotIndex 퀵슬롯에 등록된 소모품을 쓴다. 쓸 칸은 서버가 고르고, 응답은 칸을 골라 쓴 사용과 같다. */
    EResult RequestUseQuickSlot(int32 QuickSlotIndex);

    /** 판매는 상점 창이 열려 있을 때만 보낸다. 상점 창을 여닫는 쪽이 부른다. */
    void SetShopOpen(bool bOpen) { bShopOpen = bOpen; }

    /** 요청을 보내는 함수를 바꾼다. 테스트가 송신을 기록하려고 쓴다. 비우면 패킷으로 보낸다. */
    void SetRequestSender(FRequestSender InSender) { RequestSender = MoveTemp(InSender); }

private:
    /** 내 플레이어의 컨테이너에서 Address의 칸을 찾는다. 칸이 없으면 nullptr. */
    const FP1Slot* FindSlot(const FP1SlotAddress& Address) const;

    EResult DecideUse(const FP1SlotAddress& Address, OUT FRequest& OutRequest) const;
    EResult DecideUnequip(const FP1SlotAddress& Address, OUT FRequest& OutRequest) const;
    EResult DecideSell(const FP1SlotAddress& Address, OUT FRequest& OutRequest) const;

    /** 응답 대기를 함께 쓰는 요청이 응답을 기다리고 있으면 보내지 않는다. */
    EResult Send(const FRequest& Request);

    void SendPacket(const FRequest& Request) const;

    bool bShopOpen = false;

    FRequestSender RequestSender;

    //~ Pending
public:
    bool IsPending(ERequestKind Kind) const { return PendingGroups.Contains(GetPendingGroup(Kind)); }

    /** Kind의 응답이 오면 성공 여부와 무관하게 부른다. */
    void HandleResponse(ERequestKind Kind) { PendingGroups.Remove(GetPendingGroup(Kind)); }

    /** 모든 응답 대기를 푼다. 새로 입장하면 부른다. 끊기기 전에 보낸 요청의 응답은 오지 않는다. */
    void ResetPending() { PendingGroups.Reset(); }

private:
    /** 응답 대기를 함께 쓰는 요청들의 대표 종류. */
    static ERequestKind GetPendingGroup(ERequestKind Kind);

    /** 응답을 기다리는 대기의 대표 종류. */
    TSet<ERequestKind> PendingGroups;

    //~ 공통
private:
    UP1MyPlayerData* GetMyPlayerData() const;
};
