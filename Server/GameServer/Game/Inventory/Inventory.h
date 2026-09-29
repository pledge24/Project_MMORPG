#pragma once

/*----------------------
        Inventory
-----------------------*/
enum
{
    MAX_SLOTS = 32
};

class Inventory
{
public:
    Inventory(PlayerRef player);
    ~Inventory();

    bool AddItem(OUT Protocol::Slot* replicatingSlot, const Protocol::Item& itemInstance, int32 count = 1, optional<int32> setSlotId = nullopt);
    // 스택 상한을 넘는 수량은 다음 슬롯으로 나눠 넣는다. 다 넣을 수 없으면 아무것도 바꾸지 않고 false.
    // 바뀐 슬롯마다 replicatingSlots에 하나씩 추가한다.
    bool AddItem(OUT RepeatedPtrField<Protocol::Slot>* replicatingSlots, int32 templateId, int32 count = 1);
    bool RemoveItem(const Protocol::Slot& requestSlot, OUT Protocol::Slot* replicatingSlot, int32 count = 1);

    int32 FindFirstAvailableSlotId(Protocol::ItemType type, int32 templateId);

    // 저장소가 없는 ItemType이면 nullptr. GetSlot과 같은 규약이다.
    vector<bool>* GetDirtyFlags(Protocol::ItemType itemType);

    Protocol::Slot* GetSlot(Protocol::SlotType type, int32 slot_id);

    void ClearDirtyFlags();


public:
    weak_ptr<Player> _player;

private:
    // 아이템 타입 하나가 쓰는 저장소다. 슬롯 배열과 더티 플래그와 슬롯 타입을 한 곳에 두어,
    // 넣을 때와 꺼낼 때 서로 다른 표를 보다가 어긋나는 일이 생기지 않게 한다.
    struct Bag
    {
        Protocol::ItemType itemType;
        Protocol::SlotType slotType;
        RepeatedPtrField<Protocol::Slot>* slots;
        vector<bool> dirtyFlags;
    };

    // 저장소가 없는 타입이면 nullptr. 신뢰 경계 밖에서 온 SlotType도 여기서 걸러진다.
    Bag* FindBag(Protocol::ItemType itemType);
    Bag* FindBag(Protocol::SlotType slotType);

    // 아이템 데이터의 "itemType" 문자열을 ItemType으로 바꾼다. 필드가 없거나 모르는 값이면 nullopt.
    static optional<Protocol::ItemType> ToItemType(const Json& itemData);
    static bool IsValidSlotId(int32 slotId) { return slotId >= 0 && slotId < MAX_SLOTS; }

private:
    vector<Bag> _bags;
};

