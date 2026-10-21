#pragma once
#include "Game/Entities/Player.h"

/**
 * 테스트가 준비 단계에서 플레이어의 상태(레벨, 직업, 골드 등)를 직접 채운다. Player가 friend로 연다.
 * 운영 코드에는 이 상태를 바로 쓰는 경로가 없다. 레벨은 누적 경험치가, 직업과 골드는 불러오기와 거래가 정한다.
 */
struct PlayerTestAccess
{
    static Protocol::PlayerInfo& PlayerInfo(Player& player) { return *player._playerInfo; }
    static Protocol::Possession& Possession(Player& player) { return *player._possession; }
    /** 준비 단계에서 아이템을 넣고 칸을 바꾼다. 운영 코드는 Process* 요청 처리와 불러오기로만 바꾼다. */
    static InventoryComponent& Inventory(Player& player) { return *player._inventory; }
    static EquipmentComponent& Equipment(Player& player) { return *player._equipment; }
    /** 세션 없이 만든 플레이어에 계정 번호를 준다. 운영 코드는 PlayerSpawnParams의 userId로 받는다. */
    static void SetUserId(Player& player, int64 userId) { player._userId = userId; }
};

/**
 * 구역마다 slotCount칸인 인벤토리 구역 표. 플레이어에게 아이템을 넣는 테스트가 GamedataTables에 넣어 설치한다.
 * 설치하지 않으면 인벤토리의 구역마다 칸이 0개다. 운영 코드는 기획 데이터의 인벤토리 구역 표를 읽는다.
 */
inline unordered_map<int32, int32> MakeInventorySections(int32 slotCount = 32)
{
    return {
        { Protocol::ITEM_TYPE_GEAR, slotCount },
        { Protocol::ITEM_TYPE_CONSUMABLE, slotCount },
        { Protocol::ITEM_TYPE_MISCELLANEOUS, slotCount },
    };
}

/** 테스트가 엔티티 번호를 정한다. Entity가 friend로 연다. 운영 코드는 팩토리가 생성 직후에 한 번만 정한다. */
struct EntityTestAccess
{
    static void SetEntityId(Entity& entity, int64 entityId) { entity.SetEntityId(entityId); }
};
