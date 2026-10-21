#pragma once
#include "Game/Entities/EntityComponent.h"

/**
 * 플레이어가 가진 아이템의 보관소.
 *
 * 아이템을 종류별 구역의 슬롯에 보관하고, 아이템을 넣고 빼는 창구가 된다.
 * 소모품의 재사용 대기도 함께 관리한다.
 */
class InventoryComponent : public EntityComponent
{
public:
    InventoryComponent(PlayerRef owner, Protocol::Inventory* inventory);
    virtual ~InventoryComponent();

    //~ 슬롯
public:
    /**
     * 아이템을 슬롯 하나에 넣고, 바뀐 슬롯을 outSlot에 쓴다(nullptr 가능).
     * setSlotId는 범위를 확인하지 않으므로 DB 로드에서만 넘긴다.
     */
    bool AddItem(OUT Protocol::Slot* outSlot, const Protocol::Item& itemInstance, int32 count = 1, optional<int32> setSlotId = nullopt);
    /**
     * 템플릿 번호로 새 아이템을 넣는다(구매, 보상). 바뀐 슬롯마다 outSlotList에 하나씩 추가한다.
     * 다 넣을 수 없으면 아무것도 바꾸지 않고 false.
     */
    bool AddItem(OUT RepeatedPtrField<Protocol::Slot>* outSlotList, int32 templateId, int32 count = 1);
    /** DB에서 불러온 슬롯 하나를 넣는다. DB 값도 검증하고, 맞지 않으면 아무것도 바꾸지 않고 false. */
    bool LoadItem(const Protocol::Slot& loadedSlot);
    /**
     * 클라이언트가 보낸 슬롯에서 count개를 빼고, 바뀐 슬롯을 outSlot에 쓴다(nullptr 불가).
     * 슬롯이 잘못됐거나 수량이 모자라면 아무것도 바꾸지 않고 false.
     */
    bool RemoveItem(const Protocol::Slot& requestSlot, OUT Protocol::Slot* outSlot, int32 count = 1);

    /** 이 아이템을 넣을 슬롯 번호. 없으면 -1. 스택 상한은 보지 않는다. */
    int32 FindFirstAvailableSlotId(Protocol::ItemType type, int32 templateId);

    /** 클라이언트 값을 넘겨도 된다. 잘못된 슬롯이면 nullptr. */
    Protocol::Slot* GetSlot(Protocol::SlotType type, int32 slot_id);
    const Protocol::Slot* GetSlot(Protocol::SlotType type, int32 slot_id) const;

private:
    /** 인벤토리 구역 하나의 저장소. 슬롯 수는 구역의 칸 수다. */
    struct Bag
    {
        Protocol::ItemType itemType;
        Protocol::SlotType slotType;
        RepeatedPtrField<Protocol::Slot>* slots;
    };

    /** 저장소가 없는 종류면 nullptr. 클라이언트가 보낸 SlotType을 넘겨도 된다. */
    Bag* FindBag(Protocol::ItemType itemType);
    Bag* FindBag(Protocol::SlotType slotType);

    /** fromSlotId부터 찾은 첫 빈 슬롯 번호. 없으면 -1. */
    static int32 FindEmptySlotId(const Bag& bag, int32 fromSlotId = 0);

    static bool IsValidSlotId(const Bag& bag, int32 slotId) { return slotId >= 0 && slotId < bag.slots->size(); }

    vector<Bag> _bags;

    //~ 재사용 대기
public:
    /** 템플릿을 nowMs에 쓸 수 없으면 true. 표에 없는 템플릿은 false. */
    bool IsCoolingDown(int32 templateId, uint64 nowMs) const;
    void StartCooldown(int32 templateId, uint64 nowMs);

private:
    /** 소모품 템플릿 id → 마지막으로 쓴 시각(ms). 저장하지 않으므로 재접속하면 사라진다. */
    map<int32, uint64> _lastUseTimeMs;
};

