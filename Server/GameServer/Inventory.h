#pragma once

/*----------------------
        Inventory
-----------------------*/
class Inventory
{
public:
    Inventory();
    ~Inventory();

    void Init(vector<Protocol::Item>& gear, vector<Protocol::Item>& consumables, vector<Protocol::Item>& miscellaneous);

    void addItem(Protocol::Slot* updatedSlots, int32 templateId, int32 count=1);
    void removeItem(Protocol::Slot* targetSlot, Protocol::Slot* updatedSlots, int32 count=1);

private:
    /* 카테고리별 인벤토리 */
    vector<Protocol::Item> _gear;
    vector<Protocol::Item> _consumables;
    vector<Protocol::Item> _miscellaneous;

    /* 더티 플래그('': 변경 없음, 'U': 업데이트, 'I': 추가(슬롯))*/
    vector<char> gearDirtyFlags;
    vector<char> consumablesDirtyFlags;
    vector<char> miscellaneousDirtyFlags;
};

