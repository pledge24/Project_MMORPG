#pragma once
#include "Game/Entities/EntityComponent.h"
#include "Game/Entities/CombatStats.h"

/**
 * 플레이어가 입은 장비의 자리.
 *
 * 장비를 부위별 슬롯에 입고 벗기는 창구가 된다.
 * 입은 장비가 올려 주는 스탯을 알려 줄 뿐, 캐릭터의 스탯을 직접 바꾸지는 않는다.
 */
class EquipmentComponent : public EntityComponent
{
public:
    EquipmentComponent(PlayerRef owner, google::protobuf::Map<int32, Protocol::Slot>* equippedGear);
    virtual ~EquipmentComponent();

    /**
     * 장비를 아이템 표의 착용 부위에 입히고, 바뀐 슬롯을 replicatingSlot에 쓴다(nullptr 가능).
     * 장비가 아니거나 착용 조건에 맞지 않거나 그 부위가 차 있으면 아무것도 바꾸지 않고 false.
     */
    bool Equip(OUT Protocol::Slot* replicatingSlot, const Protocol::Item& itemInstance);
    /**
     * DB에서 불러온 장비를 gearType 부위에 넣는다. 아이템 표의 부위와 다르면 false.
     * 착용 조건은 보지 않으므로, 기획표의 조건이 높아져도 이미 입은 장비는 남는다.
     */
    bool LoadEquipped(const Protocol::Item& itemInstance, int32 gearType);
    /**
     * 그 부위의 장비를 벗기고, 바뀐 슬롯을 replicatingSlot에 쓴다(nullptr 가능). 빈 부위면 false.
     * 벗긴 장비를 인벤토리에 넣지 않으므로 호출자가 넣는다.
     */
    bool Unequip(int32 gearType, OUT Protocol::Slot* replicatingSlot);

    /** 착용한 장비가 올려 주는 스탯의 합. 표에 없는 장비는 더하지 않는다. */
    CombatStats SumStatDelta() const;

    /** 없는 부위면 nullptr. */
    const Protocol::Slot* GetSlot(int32 gearType) const;

    /** 부위 번호마다 저장할 변경이 있는지. 한 번도 바뀌지 않은 부위는 키가 없다. */
    map<int32, bool>& GetDirtyFlagMappings() { return _dirtyFlagMappings; }
    void ClearDirtyFlags();

private:
    /** level과 characterClass가 아이템 표의 착용 조건에 맞으면 true. */
    static bool MeetsRequirement(const ItemTemplate& itemTemplate, int32 level, Protocol::CharacterClass characterClass);
    /** 빈 부위에 아이템을 넣는다. 없는 부위이거나 차 있으면 false. */
    bool PlaceItem(OUT Protocol::Slot* replicatingSlot, const Protocol::Item& itemInstance, int32 gearType);

private:
    google::protobuf::Map<int32, Protocol::Slot>* _equippedGearLookup;
    map<int32, bool> _dirtyFlagMappings;
};
