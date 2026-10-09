#pragma once
#include "Game/Entities/EntityComponent.h"
#include "Game/Entities/CombatStats.h"

/**
 * 플레이어 한 명의 장비 장착. 장비 부위(GearType)마다 슬롯 하나를 둔다.
 * 슬롯은 플레이어의 _possession(protobuf) 안에 있고, 이 클래스는 그 포인터만 든다.
 * 스탯은 바꾸지 않는다. 착용 장비가 올려 주는 증감량만 돌려주고, 최종 스탯은 Player::RefreshFinalStat이 계산한다.
 * 룸에 들어가기 전에는 ProgressStorage::Load가 DB에서 채우고, 그 뒤로는 소속 룸 큐 위에서만 쓴다.
 */
class EquipmentComponent : public EntityComponent
{
public:
    /** owner의 _possession에 부위별 빈 슬롯을 만든다. */
    EquipmentComponent(PlayerRef owner);
    virtual ~EquipmentComponent();

    /**
     * 장착 요청을 처리한다. 아이템 표의 착용 부위에 넣는다.
     * 장비가 아니거나 그 부위가 이미 차 있으면 false. 실패하면 아무것도 바꾸지 않는다.
     */
    bool Equip(OUT Protocol::Slot* replicatingSlot, const Protocol::Item& itemInstance);
    /** DB에서 불러온 장비를 gearType 부위에 넣는다. 아이템의 부위와 같은지는 확인하지 않는다. */
    bool LoadEquipped(const Protocol::Item& itemInstance, int32 gearType);
    /**
     * 그 부위에 든 장비를 뺀다. 실패하면 아무것도 바꾸지 않는다.
     * 뺀 장비를 인벤토리에 넣지는 않는다. 호출자가 넣는다.
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
    /** 빈 부위에 아이템을 넣는다. 없는 부위이거나 차 있으면 false. */
    bool PlaceItem(OUT Protocol::Slot* replicatingSlot, const Protocol::Item& itemInstance, int32 gearType);

private:
    google::protobuf::Map<int32, Protocol::Slot>* _equippedGearLookup;
    map<int32, bool> _dirtyFlagMappings;
};
