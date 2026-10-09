#include "Core/pch.h"
#include "Network/GameEntry.h"
#include "DB/ProgressStorage.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Entities/PlayerProgress.h"

Protocol::S_ENTER_GAME GameEntry::Enter(DBConnection& conn, const GameSessionRef& session, int64 characterId)
{
    Protocol::S_ENTER_GAME enterGamePkt;
    enterGamePkt.set_success(false);

    PlayerProgress progress;
    if (ProgressStorage::Load(conn, session->_userId, characterId, OUT progress) == false)
        return enterGamePkt;

    PlayerRef player = SpawnPlayer(session, progress);
    if (player == nullptr)
        return enterGamePkt;

    enterGamePkt.set_success(true);
    enterGamePkt.mutable_player()->CopyFrom(player->GetEntityInfo());
    enterGamePkt.mutable_stat_info()->CopyFrom(player->GetStatInfo());
    enterGamePkt.mutable_possession()->CopyFrom(player->GetPossession());

    return enterGamePkt;
}

PlayerRef GameEntry::SpawnPlayer(const GameSessionRef& session, const PlayerProgress& progress)
{
    PlayerSpawnParams spawnParams;
    spawnParams.session = session;
    spawnParams.progress = &progress;

    // 팩토리는 진행을 채우고 검증까지 한다. 검증에 실패하면 nullptr이고, 그때 세션은 아직 바뀌지 않았다.
    PlayerRef player = EntityFactory::Create<Player>(spawnParams);
    if (player == nullptr)
    {
        GLogger->Warning("캐릭터 {}의 진행이 검증을 통과하지 못해 입장을 거절합니다", progress.playerInfo.character_id());
        return nullptr;
    }

    // 비어 있을 때만 등록한다. 이미 있는 플레이어를 덮어쓰면 그 플레이어는 룸에 남고, 끊길 때 퇴장하지 않는다.
    // 같은 세션의 입장 잡 둘이 다른 DB 큐에서 돌 수 있으므로 확인과 등록을 한 번에 한다.
    PlayerRef expected = nullptr;
    if (session->_player.compare_exchange_strong(expected, player) == false)
    {
        GLogger->Warning("계정 {}의 세션에 이미 플레이어가 있어 입장을 거절합니다", session->_userId);
        return nullptr;
    }

    return player;
}
