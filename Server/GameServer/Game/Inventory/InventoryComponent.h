#pragma once
#include "Game/Entities/EntityComponent.h"

enum
{
    /** 아이템 종류마다 인벤토리 슬롯 수. */
    MAX_SLOTS = 32
};

/**
 * 플레이어 한 명의 인벤토리와 소모품 재사용 대기. 아이템 종류(장비, 소모품, 기타)마다 MAX_SLOTS칸 저장소를 하나씩 둔다.
 * 슬롯은 플레이어의 _possession(protobuf) 안에 있고, 이 클래스는 그 포인터만 든다.
 * 룸에 들어가기 전에는 ProgressStorage::Load가 DB에서 채우고, 그 뒤로는 소속 룸 큐 위에서만 쓴다.
 * 바뀐 슬롯에는 더티 플래그가 찍히고, 저장할 행을 만들 때 그 플래그를 읽는다.
 */
class InventoryComponent : public EntityComponent
{
public:
    /** owner의 _possession에 빈 슬롯을 만든다. 같은 플레이어에 두 번 만들면 슬롯이 두 벌 생긴다. */
    explicit InventoryComponent(PlayerRef owner);
    virtual ~InventoryComponent();

    /**
     * 아이템 인스턴스를 슬롯 하나에 넣는다. 같은 아이템이 든 슬롯이면 수량을 더한다.
     * setSlotId를 주면 그 슬롯에 넣는다. 범위와 그 슬롯에 든 아이템은 확인하지 않는다(DB 로드용).
     * 새로 넣는 장비에 item_uid가 없으면 GNextItemUID로 붙인다. replicatingSlot이 nullptr면 복제본을 쓰지 않는다.
     */
    bool AddItem(OUT Protocol::Slot* replicatingSlot, const Protocol::Item& itemInstance, int32 count = 1, optional<int32> setSlotId = nullopt);
    /**
     * 템플릿 번호로 새 아이템을 넣는다(구매 등). 장비는 합치지 않고, 나머지는 같은 아이템이 든 슬롯부터 채운다.
     * 스택 상한을 넘는 수량은 다음 슬롯으로 나눠 넣는다. 다 넣을 수 없으면 아무것도 바꾸지 않고 false.
     * 바뀐 슬롯마다 replicatingSlots에 하나씩 추가한다. count가 0 이하면 false.
     */
    bool AddItem(OUT RepeatedPtrField<Protocol::Slot>* replicatingSlots, int32 templateId, int32 count = 1);
    /**
     * DB에서 불러온 행 하나를 그 슬롯 번호에 넣는다. DB 값을 믿지 않는다.
     * 저장소가 없는 슬롯 종류, 범위 밖 번호, 이미 찬 칸, 표에 없는 템플릿, 슬롯 종류와 다른 아이템 종류,
     * 1보다 작거나 스택 상한을 넘는 수량이면 아무것도 바꾸지 않고 false.
     */
    bool LoadItem(const Protocol::Slot& loadedSlot);
    /**
     * requestSlot의 type과 slot_id는 클라이언트 값이어도 된다. 저장소가 없거나 범위 밖이면 false.
     * 수량이 모자라면 아무것도 바꾸지 않고 false. replicatingSlot은 nullptr이면 안 된다.
     */
    bool RemoveItem(const Protocol::Slot& requestSlot, OUT Protocol::Slot* replicatingSlot, int32 count = 1);

    /**
     * 넣을 슬롯 번호를 돌려준다. 없으면 -1.
     * 장비는 가장 왼쪽 빈 슬롯, 나머지는 같은 아이템이 든 슬롯을 먼저 고른다. 스택 상한은 보지 않는다.
     */
    int32 FindFirstAvailableSlotId(Protocol::ItemType type, int32 templateId);

    //~ 재사용 대기
    /** 템플릿을 nowMs에 쓸 수 없으면 true. 템플릿마다 따로 돈다. 표에 없는 템플릿은 false. */
    bool IsCoolingDown(int32 templateId, uint64 nowMs) const;
    /** 템플릿의 재사용 대기를 nowMs부터 센다. */
    void StartCooldown(int32 templateId, uint64 nowMs);

    /** 저장소가 없는 ItemType이면 nullptr. GetSlot과 같은 규약이다. */
    vector<bool>* GetDirtyFlags(Protocol::ItemType itemType);

    /** 클라이언트 값을 받아도 된다. 저장소가 없거나 slot_id가 범위 밖이면 nullptr. */
    Protocol::Slot* GetSlot(Protocol::SlotType type, int32 slot_id);

    void ClearDirtyFlags();

private:
    /**
     * 아이템 타입 하나가 쓰는 저장소다. 슬롯 배열과 더티 플래그와 슬롯 타입을 한 곳에 두어,
     * 넣을 때와 꺼낼 때 서로 다른 표를 보다가 어긋나는 일이 생기지 않게 한다.
     */
    struct Bag
    {
        Protocol::ItemType itemType;
        Protocol::SlotType slotType;
        RepeatedPtrField<Protocol::Slot>* slots;
        vector<bool> dirtyFlags;
    };

    /** 저장소가 없는 타입이면 nullptr. 신뢰 경계 밖에서 온 SlotType도 여기서 걸러진다. */
    Bag* FindBag(Protocol::ItemType itemType);
    Bag* FindBag(Protocol::SlotType slotType);

    /** fromSlotId부터 오른쪽으로 찾은 첫 빈 슬롯 번호. 없으면 -1. 빈 슬롯은 이 함수로만 찾는다. */
    static int32 FindEmptySlotId(const Bag& bag, int32 fromSlotId = 0);

    static bool IsValidSlotId(int32 slotId) { return slotId >= 0 && slotId < MAX_SLOTS; }

private:
    vector<Bag> _bags;

    /** 소모품 템플릿 id → 마지막으로 쓴 시각(ms). 저장하지 않으므로 재접속하면 사라진다. */
    map<int32, uint64> _lastUseTimeMs;
};

