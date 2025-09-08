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

    bool Init();
    bool CalculateFinalStat();

public:
	weak_ptr<GameSession> session;

    Protocol::PlayerInfo* playerInfo;   // 플레이어의 모든 정보가 여기에 저장됨.
    Protocol::StatInfo* statInfo;

    InventoryRef inventory;             // 인벤토리 헬퍼
    EquippedGearRef equippedGear;       // 장착 아이템 헬퍼
};

