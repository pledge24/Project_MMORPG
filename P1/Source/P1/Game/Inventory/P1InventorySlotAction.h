#pragma once

#include "CoreMinimal.h"
#include "Protocol.pb.h"

/** 인벤토리 칸을 더블클릭했을 때 보낼 요청의 판정이다. */
struct P1_API FP1InventorySlotAction
{
    enum class EKind : uint8
    {
        /** 소모품을 사용한다. */
        Use,
        /** 장비를 착용한다. */
        Equip,
        /** 칸이 비었다. 요청하지 않는다. */
        Empty,
        /** 요구 레벨이 플레이어 레벨보다 높다. 요청하지 않는다. */
        LevelTooLow,
        /** 사용도 착용도 할 수 없는 칸이다. 요청하지 않는다. */
        Unusable,
        /** 소모품이 재사용 대기 중이다. 요청하지 않는다. */
        CoolingDown,
    };

    /**
     * 아이템 종류는 슬롯 종류로 가른다. 서버가 인벤토리 칸을 아이템 종류별로 나눠 두었기 때문이다.
     * TemplateId가 0 이하이면 빈 칸이다. bCoolingDown은 그 아이템이 재사용 대기 중인지이고, 소모품에만 쓴다.
     */
    static EKind Decide(Protocol::SlotType SlotType, int32 TemplateId, int32 LevelRequirement, int32 PlayerLevel, bool bCoolingDown);
};
