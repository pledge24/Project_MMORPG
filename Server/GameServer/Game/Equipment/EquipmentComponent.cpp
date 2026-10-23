#include "Core/pch.h"
#include "Game/Equipment/EquipmentComponent.h"
#include "Game/Entities/Player.h"

namespace
{
    /** 장비 부위 값 전체. GEAR_PART_NONE은 부위가 아니므로 뺀다. */
    vector<int32> MakeGearSlotIndices()
    {
        vector<int32> slotIndices;
        for (int32 gearType = Protocol::GearPart_MIN; gearType <= Protocol::GearPart_MAX; gearType++)
        {
            if (Protocol::GearPart_IsValid(gearType) && gearType != Protocol::GEAR_PART_NONE)
                slotIndices.push_back(gearType);
        }

        return slotIndices;
    }
}

EquipmentComponent::EquipmentComponent(PlayerRef owner)
    : EntityComponent(owner), _equipped(ContainerType::EQUIPPED, MakeGearSlotIndices())
{
}

EquipmentComponent::~EquipmentComponent()
{
}

ItemOutcome<Protocol::GearPart> EquipmentComponent::Equip(const ItemInstance& item)
{
    // 칸에 든 아이템은 표에 있는 아이템이므로, 표에 없으면 기획 데이터가 바뀐 것이다.
    const ItemTemplate* itemTemplate = Gamedata::FindItem(item.templateId);
    if (itemTemplate == nullptr)
        return ItemRejection{ Protocol::ITEM_RESULT_INTERNAL_ERROR };

    if (itemTemplate->gearType.has_value() == false)
        return ItemRejection{ Protocol::ITEM_RESULT_INVALID_REQUEST };

    PlayerRef owner = static_pointer_cast<Player>(_owner.lock());
    if (owner == nullptr)
        return ItemRejection{ Protocol::ITEM_RESULT_INTERNAL_ERROR };

    // 착용 조건은 클라이언트도 보지만, 판정은 서버가 한다.
    if (MeetsRequirement(*itemTemplate, owner->GetPlayerInfo().level(), owner->GetCharacterClass()) == false)
        return ItemRejection{ Protocol::ITEM_RESULT_REQUIREMENT_NOT_MET };

    // 장착 중인 부위에 다른 장비를 겹쳐 입지 않는다.
    const Protocol::GearPart gearPart = itemTemplate->gearType.value();
    if (_equipped.Find(gearPart) != nullptr)
        return ItemRejection{ Protocol::ITEM_RESULT_GEAR_PART_OCCUPIED };

    if (_equipped.Put(gearPart, item) == false)
        return ItemRejection{ Protocol::ITEM_RESULT_INTERNAL_ERROR };

    return gearPart;
}

bool EquipmentComponent::LoadEquipped(const ItemInstance& item, int32 gearType)
{
    const ItemTemplate* itemTemplate = Gamedata::FindItem(item.templateId);
    if (itemTemplate == nullptr || itemTemplate->gearType.has_value() == false)
        return false;

    // DB의 부위를 믿지 않는다. 무기가 투구 칸에 든 채로 불러오면 그대로 저장된다.
    if (itemTemplate->gearType.value() != gearType)
        return false;

    return _equipped.Put(gearType, item);
}

optional<ItemInstance> EquipmentComponent::Unequip(int32 gearType)
{
    return _equipped.Take(gearType);
}

CombatStats EquipmentComponent::SumStatDelta() const
{
    CombatStats sum;
    for (int32 gearType : _equipped.GetSlotIndices())
    {
        const ItemInstance* item = _equipped.Find(gearType);
        if (item == nullptr)
            continue;

        // 장비 칸에는 표에 있는 아이템만 들어간다(Equip과 LoadEquipped가 거른다).
        const ItemTemplate* itemTemplate = Gamedata::FindItem(item->templateId);
        if (itemTemplate == nullptr)
            continue;

        sum += CombatStats{ itemTemplate->hp, itemTemplate->mp, itemTemplate->physicalAttack, itemTemplate->magicalAttack };
    }

    return sum;
}

void EquipmentComponent::CollectItems(OUT vector<PlacedItem>& items) const
{
    for (int32 gearType : _equipped.GetSlotIndices())
    {
        if (const ItemInstance* item = _equipped.Find(gearType))
            items.push_back(PlacedItem{ SlotAddress{ ContainerType::EQUIPPED, gearType }, *item });
    }
}

bool EquipmentComponent::MeetsRequirement(const ItemTemplate& itemTemplate, int32 level, Protocol::CharacterClass characterClass)
{
    if (level < itemTemplate.levelRequirement)
        return false;

    if (itemTemplate.classRequirement.has_value() && itemTemplate.classRequirement.value() != characterClass)
        return false;

    return true;
}
