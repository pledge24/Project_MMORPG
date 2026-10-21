#pragma once

#include "CoreMinimal.h"
#include "Game/Items/P1Slot.h"

/** 슬롯 위젯 하나가 그릴 내용. 기본값은 빈 칸이다. */
struct FP1SlotDisplay
{
    /** 0 이하이면 빈 칸. */
    int32 TemplateId = 0;

    /** 2 이상일 때만 개수를 적는다. */
    int32 Count = 0;

    /** 재사용 대기를 보일 템플릿 id. 0이면 대기를 보이지 않는다. */
    int32 CooldownTemplateId = 0;

    bool IsEmpty() const { return TemplateId <= 0; }

    /** 칸이 든 아이템을 그린다. 대기는 그 아이템의 템플릿으로 본다. */
    static FP1SlotDisplay FromSlot(const FP1Slot& InSlot)
    {
        if (InSlot.HasItem() == false)
            return FP1SlotDisplay();

        return FP1SlotDisplay{ InSlot.Item.TemplateId, InSlot.Item.Count, InSlot.Item.TemplateId };
    }

    bool operator==(const FP1SlotDisplay&) const = default;
};
