#include "Core/pch.h"
#include "Network/GameEntry.h"
#include "DB/ProgressStorage.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Entities/PlayerProgress.h"
#include "Game/Room/RoomTransfer.h"
#include "Network/ItemPackets.h"

namespace
{
    // DB는 맵과 좌표만 저장한다. 들어갈 룸은 그 맵과 좌표로 고르고, 마을로 보낼 때는 맵과 위치도 마을의 것으로 바꾼다.
    bool AssignEnterRoom(PlayerProgress& progress)
    {
        const optional<RoomAssignment> assignment = RoomTransfer::AssignRoom(progress.playerInfo.map_id(), progress.posInfo);
        if (assignment.has_value() == false)
        {
            GLogger->Error("캐릭터 {}가 맵 {}에서 들어갈 룸이 없어 입장을 거절합니다(맵 표에 마을 리스폰 지점이 없다)",
                progress.playerInfo.character_id(), progress.playerInfo.map_id());
            return false;
        }

        progress.playerInfo.set_map_id(assignment->mapId);
        progress.playerInfo.set_room_id(assignment->roomId);
        progress.posInfo = assignment->pos;
        return true;
    }
}

Protocol::S_ENTER_GAME GameEntry::Enter(DBConnection& conn, const GameSessionRef& session, int64 userId, int64 characterId)
{
    Protocol::S_ENTER_GAME enterGamePkt;
    enterGamePkt.set_success(false);

    // 진행을 불러오고 들어갈 룸을 고른다.
    PlayerProgress progress;
    if (ProgressStorage::Load(conn, userId, characterId, OUT progress) == false)
        return enterGamePkt;

    if (AssignEnterRoom(progress) == false)
        return enterGamePkt;

    // 검증을 통과한 플레이어만 세션에 등록한다.
    PlayerRef player = SpawnPlayer(session, userId, progress);
    if (player == nullptr)
        return enterGamePkt;

    enterGamePkt.set_success(true);
    enterGamePkt.mutable_player()->CopyFrom(player->GetEntityInfo());
    enterGamePkt.mutable_stat_info()->CopyFrom(player->GetStatInfo());
    *enterGamePkt.mutable_possession() = ItemPackets::MakePossession(*player);

    return enterGamePkt;
}

PlayerRef GameEntry::SpawnPlayer(const GameSessionRef& session, int64 userId, const PlayerProgress& progress)
{
    PlayerSpawnParams spawnParams;
    spawnParams.session = session;
    spawnParams.userId = userId;
    spawnParams.progress = &progress;

    // 팩토리는 진행을 채우고 검증까지 한다. 검증에 실패하면 nullptr이고, 그때 세션은 아직 바뀌지 않았다.
    PlayerRef player = EntityFactory::Create<Player>(spawnParams);
    if (player == nullptr)
    {
        GLogger->Warning("캐릭터 {}의 진행이 검증을 통과하지 못해 입장을 거절합니다", progress.playerInfo.character_id());
        return nullptr;
    }

    // 비어 있을 때만 등록한다. 이미 있는 플레이어를 덮어쓰면 그 플레이어는 룸에 남고, 끊길 때 퇴장하지 않는다.
    if (session->TryRegisterPlayer(player) == false)
    {
        GLogger->Warning("계정 {}의 세션에 이미 플레이어가 있어 입장을 거절합니다", userId);
        return nullptr;
    }

    return player;
}
