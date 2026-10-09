#pragma once
#include "Game/Entities/EntityComponent.h"

/**
 * 플레이어 한 명의 장비 장착. 장비 부위(GearType)마다 슬롯 하나를 둔다.
 * 슬롯은 플레이어의 _possession(protobuf) 안에 있고, 이 클래스는 그 포인터만 든다.
 * 룸에 들어가기 전에는 ProgressStorage::Load가 DB에서 채우고, 그 뒤로는 소속 룸 큐 위에서만 쓴다.
 */
class EquipmentComponent : public EntityComponent
{
public:
    /** owner의 _possession에 부위별 빈 슬롯을 만든다. */
    EquipmentComponent(PlayerRef owner);
    virtual ~EquipmentComponent();

    /**
     * 아이템 데이터의 세부 종류로 부위를 정해 장착한다. 그 부위가 이미 차 있으면 false. 실패하면 아무것도 바꾸지 않는다.
     * setSlotId를 주면 그 부위에 넣는다. 아이템의 부위와 같은지는 확인하지 않는다(DB 로드용).
     * updatedStatList가 nullptr이면 스텟을 바꾸지 않는다. 그때는 Player::CalculateFinalStat이 장비까지 계산한다.
     */
    bool EquipGear(OUT Protocol::Slot* replicatingSlot, OUT RepeatedPtrField<Protocol::Stat>* updatedStatList, const Protocol::Item& itemInstance, optional<int32> setSlotId = nullopt);
    /**
     * 그 부위에 든 장비를 뺀다. 스텟은 뺀 장비의 수치로 계산한다. 실패하면 아무것도 바꾸지 않는다.
     * 뺀 장비를 인벤토리에 넣지는 않는다. 호출자가 넣는다.
     */
    bool UnequipGear(int32 gearType, OUT Protocol::Slot* replicatingSlot, OUT RepeatedPtrField<Protocol::Stat>* updatedStatList);

    /** 없는 부위면 nullptr. */
    const Protocol::Slot* GetSlot(int32 gearType) const;

    /** 부위 번호마다 저장할 변경이 있는지. 한 번도 바뀌지 않은 부위는 키가 없다. */
    map<int32, bool>& GetDirtyFlagMappings() { return _dirtyFlagMappings; }
    void ClearDirtyFlags();

private:
    google::protobuf::Map<int32, Protocol::Slot>* _equippedGearLookup;
    map<int32, bool> _dirtyFlagMappings;
};

