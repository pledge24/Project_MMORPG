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

    bool removeItem(OUT Protocol::Slot* updatedSlot, Protocol::Slot* targetSlot, int32 count = 1);

    int32 findFirstAvailableSlotId(Protocol::ItemType type, int32 templateId);

private:
    /* 카테고리별 인벤토리 */
    vector<Protocol::Slot> _gear;
    vector<Protocol::Slot> _consumables;
    vector<Protocol::Slot> _miscellaneous;

    /* 더티 플래그*/
    vector<Protocol::UpdateState> _gearDirtyFlags;
    vector<Protocol::UpdateState> _consumablesDirtyFlags;
    vector<Protocol::UpdateState> _miscellaneousDirtyFlags;

    unordered_map<Protocol::ItemType, vector<Protocol::Slot>&> inventoryMappings;
    unordered_map<Protocol::ItemType, vector<Protocol::UpdateState>&> dirtyFlagsMappings;
};

