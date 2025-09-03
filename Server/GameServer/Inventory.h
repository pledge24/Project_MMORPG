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
    Inventory();
    ~Inventory();

    void Init(Protocol::PlayerInfo* info);
    void addItem(Protocol::Slot* updatedSlots, int32 templateId, int32 count=1);
    void removeItem(Protocol::Slot* targetSlot, Protocol::Slot* updatedSlots, int32 count=1);

private:
    /* 카테고리별 인벤토리 */
    vector<Protocol::Slot> _gear;
    vector<Protocol::Slot> _consumables;
    vector<Protocol::Slot> _miscellaneous;

    /* 더티 플래그('': 변경 없음, 'U': 업데이트, 'I': 추가(슬롯), 'D' 삭제됨(비워진 경우 포함)*/
    vector<char> _gearDirtyFlags;
    vector<char> _consumablesDirtyFlags;
    vector<char> _miscellaneousDirtyFlags;

};

