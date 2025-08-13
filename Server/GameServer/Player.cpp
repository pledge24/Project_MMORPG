#include "pch.h"
#include "Player.h"

Player::Player()
{
	_isPlayer = true;

    playerInfo = objectInfo->mutable_player_info();
    overview = playerInfo->mutable_overview();
}

Player::~Player()
{

}
