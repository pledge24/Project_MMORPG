#pragma once

class EquippedGear
{
public:
    EquippedGear(PlayerRef player);
    ~EquippedGear();

    bool EquipGear(OUT Protocol::Slot* replicatingSlot, OUT RepeatedPtrField<Protocol::Stat>* updatedStatList, const Protocol::Item& itemInstance, optional<int32> setSlotId = nullopt);
    bool UnequipGear(const Protocol::Slot& requestSlot, OUT Protocol::Slot* replicatingSlot, OUT RepeatedPtrField<Protocol::Stat>* updatedStatList);

    map<int32, bool>& GetDirtyFlagMappings() { return dirtyFlagMappings; }
    void ClearDirtyFlag();

    weak_ptr<Player> _player;

private:
    google::protobuf::Map<int32, Protocol::Slot>* equippedGearLookup;
    map<int32, bool> dirtyFlagMappings;
    unordered_map<string, Protocol::GearType> gearTypeMappings;
};

