#pragma once

enum
{
    MAX_EQUIPPABLE_SLOT_ID = 6
};

class EquippedGear
{
public:
    EquippedGear();
    ~EquippedGear();

    void Init(Protocol::PlayerInfo* info);

    bool EquipGear(Protocol::Slot* slot, OUT Protocol::Slot* updatedSlot);
    bool UnEquipGear(Protocol::Slot* slot, OUT Protocol::Slot* updatedSlot);


private:
    vector<Protocol::Item> _gear;

    /* 더티 플래그('': 변경 없음, 'U': 업데이트, 'I': 추가(슬롯))*/
    vector<char> _gearDirtyFlags;
};

