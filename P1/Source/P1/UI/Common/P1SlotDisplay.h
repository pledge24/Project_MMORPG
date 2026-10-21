#pragma once

#include "CoreMinimal.h"
#include "Game/Items/P1ItemContainer.h"
#include "Game/Items/P1Slot.h"
#include "Game/QuickSlots/P1QuickSlotRef.h"

/** 슬롯 위젯 하나가 그릴 내용. 기본값은 빈 칸이다. */
struct FP1SlotDisplay
{
    /** 0 이하이면 빈 칸. */
    int32 TemplateId = 0;

    /** 2 이상일 때만 개수를 적는다. bAlwaysShowCount면 0과 1도 적는다. */
    int32 Count = 0;

    /** 재사용 대기를 보일 템플릿 id. 0이면 대기를 보이지 않는다. */
    int32 CooldownTemplateId = 0;

    /** 아이템을 어둡게 그린다. 다 떨어진 소모품을 가리키는 퀵슬롯이 쓴다. */
    bool bDimmed = false;

    /** 개수가 1 이하여도 적는다. */
    bool bAlwaysShowCount = false;

    bool IsEmpty() const { return TemplateId <= 0; }

    /** 칸이 든 아이템을 그린다. 대기는 그 아이템의 템플릿으로 본다. */
    static FP1SlotDisplay FromSlot(const FP1Slot& InSlot)
    {
        if (InSlot.HasItem() == false)
            return FP1SlotDisplay();

        return FP1SlotDisplay{ InSlot.Item.TemplateId, InSlot.Item.Count, InSlot.Item.TemplateId };
    }

    /**
     * 퀵슬롯이 가리키는 아이템을 그린다. 개수는 Consumables의 모든 칸에 든 그 아이템의 합이고 늘 적는다.
     * 다 떨어져도 등록은 남으므로 개수가 0이면 어둡게 그린다. Consumables가 nullptr이면 개수를 0으로 본다.
     */
    static FP1SlotDisplay FromQuickSlotRef(const FP1QuickSlotRef& Ref, const UP1ItemContainer* Consumables)
    {
        if (Ref.Type != EP1QuickSlotRefType::Item || Ref.Id <= 0)
            return FP1SlotDisplay();

        const int32 Total = Consumables ? Consumables->CountItem(Ref.Id) : 0;
        return FP1SlotDisplay{ Ref.Id, Total, Ref.Id, Total <= 0, true };
    }

    bool operator==(const FP1SlotDisplay&) const = default;
};
