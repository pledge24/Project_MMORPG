#include "Core/pch.h"
#include "Game/QuickSlots/QuickSlotComponent.h"
#include "Game/Entities/Player.h"

QuickSlotComponent::QuickSlotComponent(PlayerRef owner)
    : EntityComponent(owner), _slots(Gamedata::GetQuickSlotCount())
{
}

QuickSlotComponent::~QuickSlotComponent()
{
}

optional<vector<QuickSlot>> QuickSlotComponent::Set(int32 slotIndex, const optional<QuickSlotRef>& ref)
{
    if (IsValidIndex(slotIndex) == false)
        return nullopt;

    // NONE 참조는 빈 참조와 같다. 빈 퀵슬롯을 NONE으로 들면 저장할 때 RefType 0의 행이 생긴다.
    const optional<QuickSlotRef> registering = (ref.has_value() && ref->type != QuickSlotRefType::NONE) ? ref : nullopt;

    // 소지 여부는 보지 않는다. 다 떨어진 소모품도 등록이 남아야 다시 샀을 때 그대로 쓰고, 개수가 0인 퀵슬롯끼리도 맞바꾼다.
    if (registering.has_value() && CanRegister(registering.value()) == false)
        return nullopt;

    vector<QuickSlot> changed;
    if (_slots[slotIndex] == registering)
        return changed;

    // 같은 대상은 퀵슬롯 하나에만 둔다. 다른 퀵슬롯에 있던 대상을 등록하면 그 퀵슬롯에는 이 퀵슬롯의 이전 대상이 간다.
    const optional<int32> previousIndex = registering.has_value() ? FindIndexOf(registering.value()) : nullopt;
    const optional<QuickSlotRef> displaced = _slots[slotIndex];

    _slots[slotIndex] = registering;
    changed.push_back(QuickSlot{ slotIndex, registering });

    if (previousIndex.has_value())
    {
        _slots[previousIndex.value()] = displaced;
        changed.push_back(QuickSlot{ previousIndex.value(), displaced });
    }

    return changed;
}

bool QuickSlotComponent::Load(const QuickSlot& quickSlot)
{
    if (IsValidIndex(quickSlot.slotIndex) == false || quickSlot.ref.has_value() == false)
        return false;

    const QuickSlotRef& ref = quickSlot.ref.value();
    if (CanRegister(ref) == false || _slots[quickSlot.slotIndex].has_value() || FindIndexOf(ref).has_value())
        return false;

    _slots[quickSlot.slotIndex] = ref;
    return true;
}

vector<QuickSlot> QuickSlotComponent::GetAll() const
{
    vector<QuickSlot> quickSlots;
    quickSlots.reserve(_slots.size());
    for (int32 slotIndex = 0; slotIndex < GetSlotCount(); slotIndex++)
        quickSlots.push_back(QuickSlot{ slotIndex, _slots[slotIndex] });

    return quickSlots;
}

void QuickSlotComponent::CollectQuickSlots(OUT vector<QuickSlot>& quickSlots) const
{
    for (int32 slotIndex = 0; slotIndex < GetSlotCount(); slotIndex++)
    {
        if (_slots[slotIndex].has_value())
            quickSlots.push_back(QuickSlot{ slotIndex, _slots[slotIndex] });
    }
}

bool QuickSlotComponent::CanRegister(const QuickSlotRef& ref)
{
    // 퀵슬롯에는 소모품만 등록한다. 스킬을 등록하게 되면 여기에 참조 종류를 더한다.
    if (ref.type != QuickSlotRefType::ITEM)
        return false;

    const ItemTemplate* itemTemplate = Gamedata::FindItem(ref.id);
    return itemTemplate != nullptr && itemTemplate->itemType == Protocol::ITEM_TYPE_CONSUMABLE;
}

optional<int32> QuickSlotComponent::FindIndexOf(const QuickSlotRef& ref) const
{
    for (int32 slotIndex = 0; slotIndex < GetSlotCount(); slotIndex++)
    {
        if (_slots[slotIndex] == ref)
            return slotIndex;
    }

    return nullopt;
}
