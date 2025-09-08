#pragma once

class EquippedGear
{
public:
    EquippedGear(PlayerRef player);
    ~EquippedGear();

    void Init(Protocol::PlayerInfo* info);

    bool EquipGear(OUT Protocol::Slot* reflectSlot, OUT Protocol::StatInfo* statInfo, Protocol::Slot* slot);
    bool UnequipGear(OUT Protocol::Slot* reflectSlot, OUT Protocol::StatInfo* statInfo, Protocol::Slot* slot);

    weak_ptr<Player> _player;

private:
    RepeatedPtrField<Protocol::Slot>* equippedGearlookupTable;
    vector<bool> gearDirtyFlags;
    unordered_map<string, Protocol::gearType> gearTypeMappings;
};

