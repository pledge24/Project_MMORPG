#pragma once
#include "Game/Entities/EntityComponent.h"
#include "Game/Entities/CombatStats.h"
#include "Game/Items/ItemContainer.h"
#include "Game/Items/ItemOutcome.h"

/**
 * 플레이어가 입은 장비의 자리.
 *
 * 착용 장비 컨테이너 하나를 두고, 착용 조건에 따라 장비를 부위별 칸에 입고 벗기는 창구가 된다.
 * 입은 장비가 올려 주는 스탯을 알려 줄 뿐, 캐릭터의 스탯을 직접 바꾸지는 않는다.
 */
class EquipmentComponent : public EntityComponent
{
public:
    EquipmentComponent(PlayerRef owner);
    virtual ~EquipmentComponent();

    /**
     * 장비를 아이템 표의 착용 부위에 입히고 그 부위를 돌려준다.
     * 장비가 아니거나 착용 조건에 맞지 않거나 그 부위가 차 있으면 아무것도 바꾸지 않고 그 사유를 돌려준다.
     */
    ItemOutcome<Protocol::GearPart> Equip(const ItemInstance& item);
    /**
     * DB에서 불러온 장비를 gearType 부위에 넣는다. 아이템 표의 부위와 다르거나 그 부위가 차 있으면 false.
     * 착용 조건은 보지 않으므로, 기획표의 조건이 높아져도 이미 입은 장비는 남는다.
     */
    bool LoadEquipped(const ItemInstance& item, int32 gearType);
    /**
     * 그 부위의 장비를 벗기고 돌려준다. 빈 부위나 없는 부위면 nullopt.
     * 벗긴 장비를 인벤토리에 넣지 않으므로 호출자가 넣는다.
     */
    optional<ItemInstance> Unequip(int32 gearType);

    /** 착용한 장비가 올려 주는 스탯의 합. 표에 없는 장비는 더하지 않는다. */
    CombatStats SumStatDelta() const;

    /** 클라이언트 값을 넘겨도 된다. 없는 부위이거나 빈 부위면 nullptr. */
    const ItemInstance* FindItem(int32 gearType) const { return _equipped.Find(gearType); }
    const ItemContainer& GetContainer() const { return _equipped; }

    /** 지난번에 꺼낸 뒤로 바뀐 칸을 changes 뒤에 붙인다. */
    void TakeChanges(OUT vector<SlotChange>& changes) { _equipped.TakeChanges(changes); }
    /** 착용한 장비를 모두 부위 순서로 items 뒤에 붙인다. */
    void CollectItems(OUT vector<PlacedItem>& items) const;

private:
    /** level과 characterClass가 아이템 표의 착용 조건에 맞으면 true. */
    static bool MeetsRequirement(const DesignData::ItemRow& itemRow, int32 level, Protocol::CharacterClass characterClass);

    /** 칸 번호가 장비 부위 값이다. 부위가 없음을 뜻하는 0번 칸은 없다. */
    ItemContainer _equipped;
};
