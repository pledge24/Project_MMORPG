#pragma once
#include "Game/Entities/EntityComponent.h"
#include "Game/QuickSlots/QuickSlotTypes.h"

/**
 * 플레이어의 퀵슬롯.
 *
 * 소모품의 아이템 정의를 번호가 붙은 퀵슬롯에 등록해 두는 자리다. 아이템을 담지 않으므로 컨테이너가 아니다.
 * 같은 대상은 퀵슬롯 하나에만 등록된다.
 */
class QuickSlotComponent : public EntityComponent
{
public:
    /** 퀵슬롯 개수는 기획 데이터에서 읽는다. */
    QuickSlotComponent(PlayerRef owner);
    virtual ~QuickSlotComponent();

    /**
     * slotIndex 퀵슬롯에 ref를 등록하고 바뀐 퀵슬롯을 돌려준다. ref가 비었거나 NONE이면 해제한다. 클라이언트 값을 넘겨도 된다.
     * 같은 대상이 다른 퀵슬롯에 있으면 둘을 맞바꾼다. 범위 밖 번호이거나 소모품이 아니면 아무것도 바꾸지 않고 nullopt.
     */
    optional<vector<QuickSlot>> Set(int32 slotIndex, const optional<QuickSlotRef>& ref);
    /**
     * DB에서 불러온 등록을 넣는다. 등록할 수 없는 대상이거나, 범위 밖이거나, 이미 찬 퀵슬롯이거나,
     * 같은 대상이 다른 퀵슬롯에 있으면 false.
     */
    bool Load(const QuickSlot& quickSlot);

    int32 GetSlotCount() const { return static_cast<int32>(_slots.size()); }
    /** 모든 퀵슬롯을 번호 순서로. 빈 퀵슬롯도 싣는다. */
    vector<QuickSlot> GetAll() const;
    /** 등록된 퀵슬롯만 번호 순서로 quickSlots 뒤에 붙인다. */
    void CollectQuickSlots(OUT vector<QuickSlot>& quickSlots) const;

private:
    /** 퀵슬롯에 둘 수 있는 대상이면 true. 지금은 아이템 표에 있는 소모품뿐이다. */
    static bool CanRegister(const QuickSlotRef& ref);
    bool IsValidIndex(int32 slotIndex) const { return slotIndex >= 0 && slotIndex < GetSlotCount(); }
    /** 그 대상이 등록된 퀵슬롯 번호. 없으면 nullopt. */
    optional<int32> FindIndexOf(const QuickSlotRef& ref) const;

    /** 퀵슬롯 번호가 순번이다. */
    vector<optional<QuickSlotRef>> _slots;
};
