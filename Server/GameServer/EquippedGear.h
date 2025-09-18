#pragma once

class EquippedGear
{
public:
    EquippedGear(PlayerRef player);
    ~EquippedGear();

    bool EquipGear(OUT Protocol::Slot* reflectSlot, OUT Protocol::StatInfo* statInfo, Protocol::Item& itemInstance, optional<int32> setSlotId = nullopt);
    bool UnequipGear(OUT Protocol::Slot* reflectSlot, OUT Protocol::StatInfo* statInfo, Protocol::Slot* slot);

    map<int32, bool>& GetDirtyFlags() { return gearDirtyFlags; }

    weak_ptr<Player> _player;

private:
    google::protobuf::Map<int32, Protocol::Slot>* equippedGearLookup;
    map<int32, bool> gearDirtyFlags;
    unordered_map<string, Protocol::GearType> gearTypeMappings;
};

