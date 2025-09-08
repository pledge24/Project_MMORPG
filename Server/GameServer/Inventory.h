#pragma once

/*----------------------
        Inventory
-----------------------*/
enum
{
    MAX_SLOTS = 50
};

class Inventory
{
public:
    Inventory(PlayerRef player);
    ~Inventory();

    bool addItem(OUT Protocol::Slot* reflectSlot, Protocol::Item& itemInstance, int32 count = 1);
    bool addItem(OUT Protocol::Slot* reflectSlot, int32 templateId, int32 count = 1);

    bool removeItem(OUT Protocol::Slot* reflectSlot, Protocol::Slot* slot, int32 count = 1);

    int32 findFirstAvailableSlotId(Protocol::ItemType type, int32 templateId);

    weak_ptr<Player> _player;

private:
    unordered_map<Protocol::ItemType, RepeatedPtrField<Protocol::Slot>*> lookupMappings;
    unordered_map<Protocol::ItemType, vector<bool>> dirtyFlagsMappings;
    unordered_map<Protocol::SlotType, Protocol::ItemType> slotTypeToItemTypeMappings;
    unordered_map<string, Protocol::ItemType> itemTypeMappings;
};

