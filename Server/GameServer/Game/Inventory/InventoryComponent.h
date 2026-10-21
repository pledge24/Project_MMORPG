#pragma once
#include "Game/Entities/EntityComponent.h"
#include "Game/Items/ItemContainer.h"

/**
 * 플레이어가 가진 아이템의 보관소.
 *
 * 아이템 종류마다 구역 컨테이너 하나를 두고, 묶음 쌓기 규칙에 따라 아이템을 넣고 빼는 창구가 된다.
 * 소모품의 재사용 대기도 함께 관리한다.
 */
class InventoryComponent : public EntityComponent
{
public:
    InventoryComponent(PlayerRef owner);
    virtual ~InventoryComponent();

    //~ 칸
public:
    /**
     * 템플릿 번호로 새 아이템을 넣는다(구매, 보상). 같은 아이템의 묶음부터 채우고 남으면 빈 칸에 넣는다.
     * 다 넣을 수 없으면 아무것도 바꾸지 않고 false.
     */
    bool AddNewItem(int32 templateId, int32 count = 1);
    /**
     * 가진 적이 있는 아이템 하나를 그 종류의 구역에 넣는다(해제한 장비). 묶음 상한은 보지 않는다.
     * 넣을 칸이 없으면 false.
     */
    bool AddItem(const ItemInstance& item);
    /** DB에서 불러온 아이템 하나를 그 칸에 넣는다. DB 값도 검증하고, 맞지 않으면 아무것도 바꾸지 않고 false. */
    bool LoadItem(const SlotAddress& address, const ItemInstance& item);
    /** 칸에서 count개를 뺀다. 클라이언트 값을 넘겨도 된다. 칸이 잘못됐거나 수량이 모자라면 아무것도 바꾸지 않고 false. */
    bool RemoveItem(const SlotAddress& address, int32 count = 1);

    /** AddItem으로 이 아이템을 넣을 칸이 있으면 true. 묶음 상한은 보지 않는다. */
    bool CanAddItem(const ItemInstance& item) const;
    /** 클라이언트 값을 넘겨도 된다. 인벤토리 구역이 아니거나 잘못된 칸이거나 빈 칸이면 nullptr. */
    const ItemInstance* FindItem(const SlotAddress& address) const;
    /** 인벤토리 구역이 아니면 nullptr. */
    const ItemContainer* FindSection(ContainerType container) const;

    /** 지난번에 꺼낸 뒤로 바뀐 칸을 구역 순서대로 changes 뒤에 붙인다. */
    void TakeChanges(OUT vector<SlotChange>& changes);
    /** 아이템이 든 칸을 모두 구역 순서, 칸 번호 순서로 items 뒤에 붙인다. */
    void CollectItems(OUT vector<PlacedItem>& items) const;

private:
    /** 인벤토리 구역이 아니면 nullptr. 클라이언트 값을 넘겨도 된다. */
    ItemContainer* FindSection(ContainerType container);
    /** 그 종류의 아이템이 드는 구역. 구역이 없는 종류면 nullptr. */
    ItemContainer* FindSectionFor(Protocol::ItemType itemType);
    const ItemContainer* FindSectionFor(Protocol::ItemType itemType) const;
    /** 장비가 아닌 아이템을 쌓을, 같은 템플릿이 든 첫 칸. 없으면 nullopt. */
    static optional<int32> FindStackSlot(const ItemContainer& section, Protocol::ItemType itemType, int32 templateId);

    /** 구역 셋. 순서는 구역 대응 표의 순서다. */
    vector<ItemContainer> _sections;

    //~ 재사용 대기
public:
    /** 템플릿을 nowMs에 쓸 수 없으면 true. 표에 없는 템플릿은 false. */
    bool IsCoolingDown(int32 templateId, uint64 nowMs) const;
    void StartCooldown(int32 templateId, uint64 nowMs);

private:
    /** 소모품 템플릿 id → 마지막으로 쓴 시각(ms). 저장하지 않으므로 재접속하면 사라진다. */
    map<int32, uint64> _lastUseTimeMs;
};
