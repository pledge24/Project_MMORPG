#pragma once
#include <string_view>

class EquippedGear
{
public:
    EquippedGear(PlayerRef player);
    ~EquippedGear();

    // 실패하면 아무것도 바꾸지 않는다.
    bool EquipGear(OUT Protocol::Slot* replicatingSlot, OUT RepeatedPtrField<Protocol::Stat>* updatedStatList, const Protocol::Item& itemInstance, optional<int32> setSlotId = nullopt);
    // 그 부위에 든 장비를 뺀다. 스텟은 뺀 장비의 수치로 계산한다. 실패하면 아무것도 바꾸지 않는다.
    bool UnequipGear(int32 gearType, OUT Protocol::Slot* replicatingSlot, OUT RepeatedPtrField<Protocol::Stat>* updatedStatList);

    // 없는 부위면 nullptr.
    const Protocol::Slot* GetSlot(int32 gearType) const;

    map<int32, bool>& GetDirtyFlagMappings() { return _dirtyFlagMappings; }
    void ClearDirtyFlag();

    weak_ptr<Player> _player;

private:
    // 아이템 데이터의 세부 종류로 장비 부위를 찾는다. 장비가 아니면 nullopt.
    optional<Protocol::GearType> FindGearType(const Json& itemData) const;

private:
    google::protobuf::Map<int32, Protocol::Slot>* _equippedGearLookup;
    map<int32, bool> _dirtyFlagMappings;
    unordered_map<string_view, Protocol::GearType> _gearTypeMappings;
};

