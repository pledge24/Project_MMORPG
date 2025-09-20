#pragma once
#include "Creature.h"

class GameSession;
class Room;
class Inventory;
class EquippedGear;

class Player : public Creature
{
public:
	Player();
	virtual ~Player();

    void Init();
    bool PostInit();
    bool CalculateFinalStat();

public:
    /* 아이템 관련 */
    bool HandleBuyItem(OUT Protocol::Slot* updatedSlot, OUT int64& totalGold, int32 templateId, int32 count = 1);
    bool HandleSellItem(OUT Protocol::Slot* updatedSlot, Protocol::Slot* targetSlot, OUT int64& totalGold, int32 count = 1);
    bool HandleUseItem(OUT Protocol::S_USE_ITEM& pkt, Protocol::Slot* targetSlot);

    /* 장비 관련 */
    bool HandleEquipGear(OUT Protocol::S_EQUIP_GEAR& pkt, Protocol::Slot* targetSlot);
    bool HandleUnequipGear(OUT Protocol::S_UNEQUIP_GEAR& pkt, Protocol::Slot* targetSlot);


	weak_ptr<GameSession> session;

    Protocol::PlayerInfo* playerInfo;   // 플레이어의 모든 정보가 여기에 저장됨.
    Protocol::StatInfo* statInfo;

    InventoryRef inventory;             // 인벤토리 헬퍼
    EquippedGearRef equippedGear;       // 장착 아이템 헬퍼
};

