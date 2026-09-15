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

    bool addItem(OUT Protocol::Slot* replicatingSlot, const Protocol::Item& itemInstance, int32 count = 1, optional<int32> setSlotId = nullopt);
    bool addItem(OUT Protocol::Slot* replicatingSlot, int32 templateId, int32 count = 1);
    bool removeItem(const Protocol::Slot& requestSlot, OUT Protocol::Slot* replicatingSlot, int32 count = 1);

    int32 findFirstAvailableSlotId(Protocol::ItemType type, int32 templateId);

    // 매핑 표에 없는 ItemType이면 nullptr. GetSlot과 같은 규약이다.
    // operator[]는 없는 키를 조회하면 빈 vector를 표에 삽입하므로,
    // 그 참조를 호출자가 인덱싱하면 범위 밖 접근이 된다.
    vector<bool>* GetDirtyFlags(Protocol::ItemType itemType)
    {
        auto dirtyFlagsIt = dirtyFlagsMappings.find(itemType);
        if (dirtyFlagsIt == dirtyFlagsMappings.end())
            return nullptr;

        return &dirtyFlagsIt->second;
    }

    Protocol::Slot* GetSlot(Protocol::SlotType type, int32 slot_id);
    
    void ClearDirtyFlags();


public:
    weak_ptr<Player> _player;

private:
    /* 신뢰 경계 밖에서 온 슬롯 입력 판정 */
    // 매핑 표에 없는 SlotType이면 nullopt. 표를 바꾸지 않고 조회만 한다.
    optional<Protocol::ItemType> ToItemType(Protocol::SlotType slotType) const;
    static bool IsValidSlotId(int32 slotId) { return slotId >= 0 && slotId < MAX_SLOTS; }

private:
    unordered_map<Protocol::ItemType, RepeatedPtrField<Protocol::Slot>*> inventorylookupMappings;
    unordered_map<Protocol::ItemType, vector<bool>> dirtyFlagsMappings;

    /* 유틸 매핑 */
    unordered_map<Protocol::SlotType, Protocol::ItemType> slotTypeToItemTypeMappings;
    unordered_map<string, Protocol::ItemType> itemTypeMappings;
};

