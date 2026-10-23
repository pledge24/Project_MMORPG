#pragma once
#include "Game/Entities/Player.h"
#include "Game/Inventory/InventoryComponent.h"
#include "Game/Equipment/EquipmentComponent.h"
#include "Game/QuickSlots/QuickSlotComponent.h"

/**
 * 테스트가 준비 단계에서 플레이어의 상태(레벨, 직업, 골드 등)를 직접 채운다. Player가 friend로 연다.
 * 운영 코드에는 이 상태를 바로 쓰는 경로가 없다. 레벨은 누적 경험치가, 직업과 골드는 불러오기와 거래가 정한다.
 */
struct PlayerTestAccess
{
    static Protocol::PlayerInfo& PlayerInfo(Player& player) { return *player._playerInfo; }
    static Protocol::CharacterClass& CharacterClass(Player& player) { return player._characterClass; }
    static int64& Gold(Player& player) { return player._gold; }
    /** 준비 단계에서 아이템을 넣고 칸을 바꾼다. 운영 코드는 Process* 요청 처리와 불러오기로만 바꾼다. */
    static InventoryComponent& Inventory(Player& player) { return *player._inventory; }
    static EquipmentComponent& Equipment(Player& player) { return *player._equipment; }
    /** 준비 단계에서 불러오기를 흉내 낸다. 운영 코드는 ProcessSetQuickSlot과 불러오기로만 바꾼다. */
    static QuickSlotComponent& QuickSlots(Player& player) { return *player._quickSlots; }
    /** 준비 단계에서 바뀐 칸을 버린다. 남겨 두면 다음 요청의 결과에 함께 실린다. */
    static void DiscardItemChanges(Player& player) { player.TakeItemChanges(); }

    /**
     * 템플릿으로 새 아이템을 인벤토리에 넣고, 처음 바뀐 칸의 주소를 돌려준다. 넣지 못하면 nullopt.
     * 준비 단계라 바뀐 칸은 버린다.
     */
    static optional<SlotAddress> AddNewItem(Player& player, int32 templateId, int32 count = 1)
    {
        if (player._inventory->AddNewItem(templateId, count) == false)
            return nullopt;

        vector<SlotChange> changes = player.TakeItemChanges();
        if (changes.empty())
            return nullopt;

        return changes.front().address;
    }

    /** 세션 없이 만든 플레이어에 계정 번호를 준다. 운영 코드는 PlayerSpawnParams의 userId로 받는다. */
    static void SetUserId(Player& player, int64 userId) { player._userId = userId; }
};

/** 그 칸에 든 아이템을 그대로 가리키는 요청. 클라이언트가 자기 칸의 주소와 uid를 보내는 것과 같다. 빈 칸이면 uid 없이 가리킨다. */
inline ItemSlotRequest MakeSlotRequest(const SlotAddress& address, const ItemInstance* item)
{
    ItemSlotRequest request;
    request.address = address;
    if (item != nullptr)
        request.itemUid = item->itemUid;

    return request;
}

/** 인벤토리 칸을 가리키는 요청. */
inline ItemSlotRequest MakeInventoryRequest(const Player& player, const SlotAddress& address)
{
    return MakeSlotRequest(address, player.GetInventory().FindItem(address));
}

/** 착용 장비 칸을 가리키는 요청. */
inline ItemSlotRequest MakeEquippedRequest(const Player& player, int32 gearType)
{
    return MakeSlotRequest(SlotAddress{ ContainerType::EQUIPPED, gearType }, player.GetEquipment().FindItem(gearType));
}

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
