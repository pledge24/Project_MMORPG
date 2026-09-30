#pragma once
#include "PlayerSaveData.h"

/*--------------------------------------------------------------
    ItemSaveRows

    접속 종료 때 아이템 테이블에 반영할 행을 고른다. DB를 모른다.
    더티 플래그가 켜진 슬롯만 행이 된다. 비워진 슬롯은 template_id 0인 행이 되고,
    저장 쿼리가 그 행을 DB에서 지운다.
---------------------------------------------------------------*/

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

// 개수로 쌓이는 아이템(소모품, 기타)의 행이다.
struct StackableItemSaveRow
{
    int64 characterId = 0;
    int32 slotId = 0;
    int32 templateId = 0;
    int32 count = 0;
};

namespace ItemSaveRows
{
    // 인벤토리의 장비와 착용 장비에서 고른다. 인벤토리 장비의 더티 플래그가 없으면 nullopt.
    optional<vector<GearSaveRow>> BuildGearRows(const PlayerSaveData& data);

    // 소모품이나 기타 아이템에서 고른다. 그 종류의 더티 플래그가 없거나 쌓이는 종류가 아니면 nullopt.
    optional<vector<StackableItemSaveRow>> BuildStackableRows(const PlayerSaveData& data, Protocol::ItemType itemType);
}
