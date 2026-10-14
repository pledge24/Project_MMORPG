#pragma once
#include "Game/Entities/PlayerSaveData.h"

/** 장비 테이블에 반영할 행 하나. template_id가 0이면 비워진 슬롯이고, 저장 쿼리가 그 행을 DB에서 지운다. */
struct GearSaveRow
{
    int64 characterId = 0;
    int32 slotId = 0;
    int64 itemUid = 0;
    int32 templateId = 0;
    bool isEquipped = false;
    int32 enhance = 0;
    int32 durability = 0;
    int32 additionalPhysicalAttack = 0;
    int32 additionalMagicalAttack = 0;
};

/** 개수로 쌓이는 아이템(소모품, 기타)의 행 하나. template_id가 0이면 비워진 슬롯이다. */
struct StackableItemSaveRow
{
    int64 characterId = 0;
    int32 slotId = 0;
    int32 templateId = 0;
    int32 count = 0;
};

/**
 * 아이템 저장 행을 고르는 규칙.
 *
 * 저장 사본에서 바뀐 슬롯만 골라 아이템 테이블에 반영할 행으로 만든다. DB에는 손대지 않는다.
 */
namespace ItemSaveRows
{
    /** 인벤토리의 장비와 착용 장비에서 고른다. 인벤토리 장비의 더티 플래그가 없으면 nullopt. */
    optional<vector<GearSaveRow>> BuildGearRows(const PlayerSaveData& data);

    /** 소모품이나 기타 아이템에서 고른다. 그 종류의 더티 플래그가 없거나 쌓이는 종류가 아니면 nullopt. */
    optional<vector<StackableItemSaveRow>> BuildStackableRows(const PlayerSaveData& data, Protocol::ItemType itemType);
}
