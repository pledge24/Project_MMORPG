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

    vector<bool>& GetDirtyFlags(Protocol::ItemType itemType) { return dirtyFlagsMappings[itemType]; }
    Protocol::Slot* GetSlot(Protocol::SlotType type, int32 slot_id);
    
    void ClearDirtyFlags();


public:
    weak_ptr<Player> _player;

private:
    unordered_map<Protocol::ItemType, RepeatedPtrField<Protocol::Slot>*> inventorylookupMappings;
    unordered_map<Protocol::ItemType, vector<bool>> dirtyFlagsMappings;

    /* 유틸 매핑 */
    unordered_map<Protocol::SlotType, Protocol::ItemType> slotTypeToItemTypeMappings;
    unordered_map<string, Protocol::ItemType> itemTypeMappings;
};

