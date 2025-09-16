#include "pch.h"
#include "ObjectUtils.h"
#include "Player.h"
#include "GameSession.h"
#include "Inventory.h"
#include "EquippedGear.h"

atomic<int64> ObjectUtils::s_idGenerator = 1;

PlayerRef ObjectUtils::CreatePlayer(GameSessionRef session)
{
	// ID 생성기(원래는 이것저것 낑겨넣음)
	const int64 newId = s_idGenerator.fetch_add(1);

	PlayerRef player = make_shared<Player>();
    player->Init();

	player->objectInfo->set_object_id(newId);
	player->posInfo->set_object_id(newId);

	player->session = session;
	session->player.store(player);

	return player;
}
