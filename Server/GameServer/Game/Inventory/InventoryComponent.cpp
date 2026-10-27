#include "Core/pch.h"
#include "Game/Inventory/InventoryComponent.h"
#include "Game/Entities/Player.h"
#include "Game/Items/InventorySections.h"
#include "Utils/SnowflakeIdGenerator.h"

InventoryComponent::InventoryComponent(PlayerRef owner) : EntityComponent(owner)
{
    // 모든 구역의 칸 수가 같다. 클라이언트 UI가 구역마다 같은 칸 수를 고정으로 그린다.
    for (const InventorySection& section : InventorySections::ALL)
        _sections.emplace_back(section.container, InventorySections::SLOT_COUNT);
}

InventoryComponent::~InventoryComponent()
{
}

bool InventoryComponent::AddNewItem(int32 templateId, int32 count)
{
    if (count <= 0)
        return false;

    const DesignData::ItemRow* itemRow = Gamedata::FindItem(templateId);
    if (itemRow == nullptr)
        return false;

    ItemContainer* section = FindSectionFor(itemRow->Type);
    if (section == nullptr)
        return false;

    const int32 maxStack = itemRow->MaxStack;

    // 넣을 자리를 먼저 다 정한다. 칸을 바꾸기 시작한 뒤에 모자라면 되돌릴 수 없다.
    vector<pair<int32, int32>> stacks; // <칸 번호, 더할 수량>
    vector<pair<int32, int32>> puts;   // <칸 번호, 놓을 수량>
    int32 remaining = count;

    // 장비는 합치지 않는다. 나머지는 같은 아이템이 든 칸의 남은 자리부터 채운다.
    if (itemRow->Type != Protocol::ITEM_TYPE_GEAR)
    {
        for (int32 slotIndex : section->GetSlotIndices())
        {
            if (remaining <= 0)
                break;

            const ItemInstance* item = section->Find(slotIndex);
            if (item == nullptr || item->templateId != templateId)
                continue;

            const int32 space = maxStack - item->count;
            if (space <= 0)
                continue;

            const int32 amount = (std::min)(space, remaining);
            stacks.emplace_back(slotIndex, amount);
            remaining -= amount;
        }
    }

    for (optional<int32> slotIndex = section->FindEmptySlot(); slotIndex.has_value() && remaining > 0; slotIndex = section->FindEmptySlot(slotIndex.value() + 1))
    {
        const int32 amount = (std::min)(maxStack, remaining);
        puts.emplace_back(slotIndex.value(), amount);
        remaining -= amount;
    }

    if (remaining > 0)
        return false;

    for (const auto& [slotIndex, amount] : stacks)
        section->AddCount(slotIndex, amount);

    for (const auto& [slotIndex, amount] : puts)
    {
        // TODO: 장비의 초기 세부 값(강화, 내구도)을 정하는 규칙이 생기면 여기서 채운다.
        ItemInstance item;
        item.itemUid = GIdGenerator->Next();
        item.templateId = templateId;
        item.count = amount;
        if (itemRow->Type == Protocol::ITEM_TYPE_GEAR)
            item.gear = GearDetail{};

        section->Put(slotIndex, item);
    }

    return true;
}

bool InventoryComponent::AddItem(const ItemInstance& item)
{
    const DesignData::ItemRow* itemRow = Gamedata::FindItem(item.templateId);
    if (itemRow == nullptr)
        return false;

    ItemContainer* section = FindSectionFor(itemRow->Type);
    if (section == nullptr)
        return false;

    // 이미 있는 묶음에 더한 개수는 그 묶음의 UID를 따른다.
    if (optional<int32> stackSlot = FindStackSlot(*section, itemRow->Type, item.templateId))
        return section->AddCount(stackSlot.value(), item.count);

    optional<int32> emptySlot = section->FindEmptySlot();
    if (emptySlot.has_value() == false)
        return false;

    // 빈 칸에 새로 놓이는 아이템은 종류와 상관없이 UID를 받는다.
    ItemInstance placed = item;
    if (placed.itemUid == 0)
        placed.itemUid = GIdGenerator->Next();

    return section->Put(emptySlot.value(), placed);
}

bool InventoryComponent::LoadItem(const SlotAddress& address, const ItemInstance& item)
{
    // 구역의 칸 수를 넘는 번호는 기획 데이터가 칸 수를 줄였거나 DB가 어긋난 것이다.
    ItemContainer* section = FindSection(address.container);
    if (section == nullptr || section->IsValidSlot(address.slotIndex) == false)
        return false;

    // 같은 칸의 두 행을 합치면 다음 저장이 한 행만 남긴다.
    if (section->Find(address.slotIndex) != nullptr)
        return false;

    const DesignData::ItemRow* itemRow = Gamedata::FindItem(item.templateId);
    if (itemRow == nullptr || InventorySections::FindContainer(itemRow->Type) != address.container)
        return false;

    if (item.count < 1 || item.count > itemRow->MaxStack)
        return false;

    ItemInstance placed = item;
    if (placed.itemUid == 0)
        placed.itemUid = GIdGenerator->Next();

    return section->Put(address.slotIndex, placed);
}

bool InventoryComponent::RemoveItem(const SlotAddress& address, int32 count)
{
    // 주소는 클라이언트 값이다. 구역과 칸 번호는 컨테이너가 거르지만, 거른 사유를 남긴다.
    ItemContainer* section = FindSection(address.container);
    if (section == nullptr)
    {
        GLogger->Warning("RemoveItem: 인벤토리 구역이 아닌 컨테이너({})", static_cast<int32>(address.container));
        return false;
    }

    if (section->IsValidSlot(address.slotIndex) == false)
    {
        GLogger->Warning("RemoveItem: 범위 밖 칸 번호({})", address.slotIndex);
        return false;
    }

    return section->RemoveCount(address.slotIndex, count);
}

bool InventoryComponent::CanAddItem(const ItemInstance& item) const
{
    const DesignData::ItemRow* itemRow = Gamedata::FindItem(item.templateId);
    if (itemRow == nullptr)
        return false;

    const ItemContainer* section = FindSectionFor(itemRow->Type);
    if (section == nullptr)
        return false;

    return FindStackSlot(*section, itemRow->Type, item.templateId).has_value() || section->FindEmptySlot().has_value();
}

const ItemInstance* InventoryComponent::FindItem(const SlotAddress& address) const
{
    const ItemContainer* section = FindSection(address.container);
    if (section == nullptr)
        return nullptr;

    return section->Find(address.slotIndex);
}

const ItemContainer* InventoryComponent::FindSection(ContainerType container) const
{
    return const_cast<InventoryComponent*>(this)->FindSection(container);
}

void InventoryComponent::TakeChanges(OUT vector<SlotChange>& changes)
{
    for (ItemContainer& section : _sections)
        section.TakeChanges(changes);
}

void InventoryComponent::CollectItems(OUT vector<PlacedItem>& items) const
{
    for (const ItemContainer& section : _sections)
    {
        for (int32 slotIndex : section.GetSlotIndices())
        {
            if (const ItemInstance* item = section.Find(slotIndex))
                items.push_back(PlacedItem{ SlotAddress{ section.GetType(), slotIndex }, *item });
        }
    }
}

ItemContainer* InventoryComponent::FindSection(ContainerType container)
{
    for (ItemContainer& section : _sections)
    {
        if (section.GetType() == container)
            return &section;
    }

    return nullptr;
}

ItemContainer* InventoryComponent::FindSectionFor(Protocol::ItemType itemType)
{
    optional<ContainerType> container = InventorySections::FindContainer(itemType);
    if (container.has_value() == false)
        return nullptr;

    return FindSection(container.value());
}

const ItemContainer* InventoryComponent::FindSectionFor(Protocol::ItemType itemType) const
{
    return const_cast<InventoryComponent*>(this)->FindSectionFor(itemType);
}

optional<int32> InventoryComponent::FindStackSlot(const ItemContainer& section, Protocol::ItemType itemType, int32 templateId)
{
    // 장비는 합치지 않는다.
    if (itemType == Protocol::ITEM_TYPE_GEAR)
        return nullopt;

    return section.FindFirstSlotOf(templateId);
}

bool InventoryComponent::IsCoolingDown(int32 templateId, uint64 nowMs) const
{
    // 재사용 대기는 소모품의 사용 효과에만 있다.
    const DesignData::ItemConsumeEffectRow* consumeEffect = Gamedata::FindItemConsumeEffect(templateId);
    if (consumeEffect == nullptr)
        return false;

    auto lastUseIt = _lastUseTimeMs.find(templateId);
    return lastUseIt != _lastUseTimeMs.end() && nowMs < lastUseIt->second + static_cast<uint64>(consumeEffect->CooldownMs);
}

void InventoryComponent::StartCooldown(int32 templateId, uint64 nowMs)
{
    _lastUseTimeMs[templateId] = nowMs;
}
