#include "pch.h"
#include "ObjectUtils.h"
#include "Player.h"
#include "Monster.h"
#include "GameSession.h"

atomic<int64> ObjectUtils::s_idGenerator = 1;

PlayerRef ObjectUtils::CreatePlayer(GameSessionRef session)
{
    PlayerRef player = static_pointer_cast<Player>(ObjectUtils::Create<Player>());
    if (player)
    {
        // objectId 생성
	    const int64 newId = s_idGenerator.fetch_add(1);

        // 추가 세팅
        {
            player->objectInfo->set_object_type(Protocol::ObjectType::OBJECT_TYPE_PLAYER);
            player->objectInfo->set_object_id(newId);
            player->posInfo->set_object_id(newId);

            player->session = session;
            session->player.store(player);
        }
    }

	return player;
}

MonsterRef ObjectUtils::CreateMonster(int32 templateId)
{
    MonsterRef monster = static_pointer_cast<Monster>(ObjectUtils::Create<Monster>());
    if (monster)
    {
        // objectId 생성
        const int64 newId = s_idGenerator.fetch_add(1);

        // 추가 세팅
        {
            monster->objectInfo->set_object_type(Protocol::ObjectType::OBJECT_TYPE_MONSTER);
            monster->objectInfo->set_object_id(newId);
            monster->posInfo->set_object_id(newId);

            // MonsterInfo templateId만 세팅
            Protocol::MonsterInfo* monsterInfo = monster->objectInfo->mutable_monster_info();
            monsterInfo->set_template_id(templateId);
        }
    }

    return monster;
}
