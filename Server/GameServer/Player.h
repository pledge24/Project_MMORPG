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

    Protocol::PlayerInfo* playerInfo;
    Protocol::StatInfo* statInfo;

    InventoryRef inventory;
    EquippedGearRef equippedGear;
};

