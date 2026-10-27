#pragma once
#include "Game/Data/Templates.h"

/**
 * 테스트가 기획 데이터 표에 아이템 행을 넣는다.
 * 실제 표처럼 장비에는 장비 능력치 행을, 소모품에는 소모 효과 행을 함께 넣는다.
 * 생성된 행의 MaxStack 기본값 0은 실제 표에 올 수 없는 값이라(DataBuilder가 막는다) 여기서 채운다.
 */
namespace TestItemTables
{
    inline DesignData::ItemRow& AddItem(GamedataTables& tables, int32 itemId, Protocol::ItemType itemType, int32 maxStack = 1)
    {
        DesignData::ItemRow& item = tables.items[itemId];
        item.Id = itemId;
        item.Type = itemType;
        item.MaxStack = maxStack;
        return item;
    }

    /** 장비 아이템을 넣고 그 장비 능력치 행을 돌려준다. 공통 열은 tables.items[itemId]로 고친다. */
    inline DesignData::ItemEquipStatRow& AddGear(GamedataTables& tables, int32 itemId, Protocol::GearPart part)
    {
        AddItem(tables, itemId, Protocol::ITEM_TYPE_GEAR);

        DesignData::ItemEquipStatRow& equipStat = tables.itemEquipStats[itemId];
        equipStat.ItemId = itemId;
        equipStat.Part = part;
        return equipStat;
    }

    /** 소모품 아이템을 넣고 그 소모 효과 행을 돌려준다. 공통 열은 tables.items[itemId]로 고친다. */
    inline DesignData::ItemConsumeEffectRow& AddConsumable(GamedataTables& tables, int32 itemId, int32 maxStack)
    {
        AddItem(tables, itemId, Protocol::ITEM_TYPE_CONSUMABLE, maxStack);

        DesignData::ItemConsumeEffectRow& consumeEffect = tables.itemConsumeEffects[itemId];
        consumeEffect.ItemId = itemId;
        return consumeEffect;
    }
}
