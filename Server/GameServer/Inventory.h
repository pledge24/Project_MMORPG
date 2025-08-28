#pragma once
class Inventory
{
public:
    Inventory();
    ~Inventory();

    void Init();

    void addItem(int32 slotId, int32 templateId, int32 count=1);
    void removeItem(int32 slotId, int32 templateId, int32 count = 1);
    void moveItem(int32 fromSlotId, int32 toSlotId, int32 templateId);

private:
    /* 카테고리별 인벤토리 */
    vector<int32> gear;
    vector<int32> consumables;
    vector<int32> miscellaneous;

    /* 더티 플래그 */
    vector<char> gearDirtyFlags;
    vector<char> consumablesDirtyFlags;
    vector<char> miscellaneousDirtyFlags;
};

