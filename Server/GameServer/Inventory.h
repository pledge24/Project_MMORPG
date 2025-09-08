#pragma once

/*----------------------
        Inventory
-----------------------*/
enum
{
    MAX_SLOTS = 40
};

class Inventory
{
public:
    Inventory();
    ~Inventory();

    void Init(Protocol::PlayerInfo* info);
    bool addItem(OUT Protocol::Slot* updatedSlot, Protocol::Item& itemInstance, int32 count = 1);
    bool addItem(OUT Protocol::Slot* updatedSlot, int32 templateId, int32 count = 1);

    bool removeItem(OUT Protocol::Slot* updatedSlot, Protocol::Slot* slot, int32 count = 1);

    int32 findFirstAvailableSlotId(Protocol::ItemType type, int32 templateId);

    weak_ptr<Player> player;

private:
    unordered_map<Protocol::ItemType, vector<Protocol::Slot*>> lookupTableMappings;
    unordered_map<Protocol::ItemType, vector<bool>> dirtyFlagsMappings;
    unordered_map<Protocol::SlotType, Protocol::ItemType> slotTypeToItemTypeMappings;
};

