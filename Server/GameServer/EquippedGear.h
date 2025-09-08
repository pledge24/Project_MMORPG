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

    bool EquipGear(OUT Protocol::Slot* updatedSlot, OUT Protocol::Stat* updatedStat, Protocol::Slot* slot);
    bool UnequipGear(OUT Protocol::Slot* updatedSlot, OUT Protocol::Stat* updatedStat, Protocol::Slot* slot);

    weak_ptr<Player> player;

private:
    vector<Protocol::Item> _gear;

    /* 더티 플래그('': 변경 없음, 'U': 업데이트, 'I': 추가(슬롯))*/
    vector<char> _gearDirtyFlags;
};

