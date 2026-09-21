#include "pch.h"
#include "ObjectUtils.h"
#include "Player.h"
#include "Monster.h"
#include "GameSession.h"

atomic<int64> ObjectUtils::s_idGenerator = 1;

PlayerRef ObjectUtils::CreatePlayer(GameSessionRef session)
{
    PlayerRef player = static_pointer_cast<Player>(ObjectUtils::Create<Player>());
    if (player == nullptr)
        return nullptr;

    // entityId 생성
    const int64 newId = s_idGenerator.fetch_add(1);

    // 추가 세팅
    {
        player->_entityInfo->set_entity_type(Protocol::EntityType::ENTITY_TYPE_PLAYER);
        player->_entityInfo->set_entity_id(newId);
        player->_posInfo->set_entity_id(newId);

        player->_session = session;
        session->_player.store(player);
    }

    // 세팅이 모두 끝난 뒤에 초기화한다.
    if (player->Init() == false)
        return nullptr;

	return player;
}

MonsterRef ObjectUtils::CreateMonster(int32 templateId, const Protocol::PosInfo& spawnPos)
{
    MonsterRef monster = static_pointer_cast<Monster>(ObjectUtils::Create<Monster>());
    if (monster == nullptr)
        return nullptr;

    // entityId 생성
    const int64 newId = s_idGenerator.fetch_add(1);

    // 추가 세팅
    {
        monster->_entityInfo->set_entity_type(Protocol::EntityType::ENTITY_TYPE_MONSTER);
        monster->_entityInfo->set_entity_id(newId);

        // MonsterInfo templateId만 세팅
        Protocol::MonsterInfo* monsterInfo = monster->_entityInfo->mutable_monster_info();
        monsterInfo->set_template_id(templateId);

        // Monster::Init()이 posInfo로 _spawnPos를 계산하므로 Init 이전에 넣어야 한다.
        monster->SetPosInfo(spawnPos);
        monster->_posInfo->set_entity_id(newId);
    }

    // template_id와 posInfo가 모두 채워진 뒤에 초기화한다.
    if (monster->Init() == false)
        return nullptr;

    return monster;
}
