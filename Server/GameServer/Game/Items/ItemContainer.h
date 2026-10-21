#pragma once
#include "Game/Items/ItemTypes.h"

/**
 * 칸 번호로 아이템을 넣고 빼는 범용 칸 배열.
 *
 * 칸 배열과 유효한 칸 번호를 들고, 칸 주소를 검증하고, 응답에 실을 바뀐 칸을 모은다.
 * 어떤 아이템이 들어갈 수 있는지는 판단하지 않는다. 바뀐 칸 목록은 응답용이고 DB 저장에 쓰지 않는다.
 */
class ItemContainer
{
public:
    /** 칸 번호가 0부터 slotCount - 1까지인 컨테이너. */
    ItemContainer(ContainerType type, int32 slotCount);
    /** slotIndices에 있는 칸 번호만 유효한 컨테이너. */
    ItemContainer(ContainerType type, const vector<int32>& slotIndices);

    //~ 칸
public:
    ContainerType GetType() const { return _type; }
    /** 유효한 칸 번호. 오름차순이다. */
    vector<int32> GetSlotIndices() const;
    bool IsValidSlot(int32 slotIndex) const { return _slots.contains(slotIndex); }
    /** 유효하지 않거나 빈 칸이면 nullptr. 클라이언트 값을 넘겨도 된다. */
    const ItemInstance* Find(int32 slotIndex) const;
    /** fromSlotIndex 이상의 칸 번호 중 가장 작은 빈 칸. 없으면 nullopt. */
    optional<int32> FindEmptySlot(int32 fromSlotIndex = INT32_MIN) const;
    /** 그 템플릿의 아이템이 든 칸 중 가장 작은 칸 번호. 없으면 nullopt. */
    optional<int32> FindFirstSlotOf(int32 templateId) const;

    /** 빈 칸에 넣는다. 유효하지 않거나 차 있거나 개수가 1보다 작으면 아무것도 바꾸지 않고 false. */
    bool Put(int32 slotIndex, const ItemInstance& item);
    /** 칸을 비우고 든 아이템을 돌려준다. 유효하지 않거나 비어 있으면 nullopt. */
    optional<ItemInstance> Take(int32 slotIndex);
    /** 든 아이템의 개수를 amount만큼 늘린다. 묶음 상한은 보지 않는다. 빈 칸이거나 amount가 1보다 작으면 false. */
    bool AddCount(int32 slotIndex, int32 amount);
    /** 든 아이템의 개수를 amount만큼 줄이고, 0이 되면 칸을 비운다. 모자라거나 amount가 1보다 작으면 false. */
    bool RemoveCount(int32 slotIndex, int32 amount);

private:
    /** 칸 번호 → 그 칸의 아이템. 빈 칸은 nullopt다. */
    map<int32, optional<ItemInstance>> _slots;
    ContainerType _type;

    //~ 바뀐 칸
public:
    /** 지난번에 꺼낸 뒤로 바뀐 칸의 지금 내용을 처음 바뀐 순서대로 changes 뒤에 붙이고 목록을 비운다. */
    void TakeChanges(OUT vector<SlotChange>& changes);

private:
    void MarkChanged(int32 slotIndex);

    /** 바뀐 칸 번호. 한 칸은 한 번만 든다. */
    vector<int32> _changedSlots;
};
