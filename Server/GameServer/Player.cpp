#include "pch.h"
#include "Player.h"
#include "Inventory.h"
#include "EquippedGear.h"

Player::Player()
{
	_isPlayer = true;

    playerInfo = objectInfo->mutable_player_info();
    inventory = make_shared<Inventory>();
    equippedGear = make_shared<EquippedGear>();
}

Player::~Player()
{
}

bool Player::Init()
{
    // inventory 채우기
    inventory->Init(playerInfo);

    // equippedGear 채우기
    equippedGear->Init(playerInfo);

}
